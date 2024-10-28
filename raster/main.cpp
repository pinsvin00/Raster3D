#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm>
#include <functional>

struct Triangle
{
    Triangle(sf::Vector2i p1, sf::Vector2i p2, sf::Vector2i p3)
        : mP1(p1), mP2(p2), mP3(p3)
    {
    }
    sf::Vector2i mP1;
    sf::Vector2i mP2;
    sf::Vector2i mP3;
};


class Rasterizer 
{
private:
    sf::Image* mBitmap;
    bool mDebugColors;
    std::function<sf::Color(int, int)> mColorCb;

public:

    void DrawLine(uint16_t sx, uint16_t sy, uint16_t ex, const sf::Color& color)
    {
        ex = std::min(static_cast<uint16_t>(mBitmap->getSize().x), ex);

        for (uint16_t cx = sx; cx < ex; cx++)
        {
            mBitmap->setPixel(cx, sy, color);
        }
    }

    void DrawLine(uint16_t sx, uint16_t sy, uint16_t ex)
    {
        ex = std::min(static_cast<uint16_t>(mBitmap->getSize().x), ex);

        for (uint16_t cx = sx; cx < ex; cx++)
        {
            mBitmap->setPixel(cx, sy, mColorCb(cx, sy));
        }
    }

    inline static int LerpVer(const sf::Vector2i& A, const sf::Vector2i distance, int y)
    {
        int dis = y - A.y;
        return A.x + (distance.x) * dis / distance.y;
    }

    void DrawTriangle(Triangle t)
    {
        //sort so we got the points on the top, as p1..p2, 
        if (t.mP2.y > t.mP3.y) std::swap(t.mP2, t.mP3);
        if (t.mP1.y > t.mP2.y) std::swap(t.mP1, t.mP2);
        if (t.mP2.y > t.mP3.y) std::swap(t.mP2, t.mP3);

        //pick the longer side
        std::pair<sf::Vector2i&, sf::Vector2i> longerPair = {
            t.mP1,
            //std::abs(t.mP1.x - t.mP2.x) + std::abs(t.mP1.y - t.mP2.y) > std::abs(t.mP1.x - t.mP3.x) + std::abs(t.mP1.y - t.mP3.y) ?
            t.mP3,
        };


        const sf::Vector2i longerSideDistance = t.mP1 - t.mP3;
        const sf::Vector2i shorterSideDistance = t.mP1 - t.mP2;
        const sf::Vector2i bottomSideDistance = t.mP2 - t.mP3;

        for (size_t i = t.mP1.y; i < t.mP2.y; i++)
        {
            int sx = LerpVer(t.mP1, longerSideDistance, i);
            int ex = LerpVer(t.mP2, shorterSideDistance, i);
            if (sx > ex)
                std::swap(sx, ex);

           DrawLine(sx, i, ex);

        }
        for (size_t i = t.mP2.y; i < t.mP3.y; i++)
        {
            int sx = LerpVer(t.mP1, longerSideDistance, i);
            int ex = LerpVer(t.mP2, bottomSideDistance, i);
            if (sx > ex)
                std::swap(sx, ex);

            DrawLine(sx, i, ex);
        }
    }

    Rasterizer(sf::Image* sourceImage, std::function<sf::Color(int, int)> colorCb = [](int x, int y) {return sf::Color::Green; }, bool useDebugColors = false)
        : mBitmap(sourceImage),
          mColorCb(colorCb),
          mDebugColors(useDebugColors)
    {

    }
    ~Rasterizer()
    {
    }
};

int main()
{

    sf::Vector2i A(7, 0);
    sf::Vector2i B(12, 11);
    sf::Vector2i C(1, 8);
    sf::RenderWindow window(sf::VideoMode(1600, 900), "Basic renderer test");

    A *= 20;
    B *= 20;
    C *= 20;


    sf::Image canvasBuffer;
    canvasBuffer.create(1600, 900, sf::Color::White);

    Triangle triangle = { C,B,A };
    Rasterizer rast(&canvasBuffer, [&](int x, int y) {
        return sf::Color::Red;
    });

    auto trianglePositions = { &triangle.mP1, &triangle.mP2, &triangle.mP3};

    rast.DrawTriangle(triangle);

    sf::Texture texture;
    texture.loadFromImage(canvasBuffer);
    sf::Sprite mySprite(texture);

    auto Distance = [&](const sf::Vector2i& a, const sf::Vector2i& b)
        {
            return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
        };

    auto ClearCanvas = [](sf::Image& canvas)
        {
            canvas.create(1600, 900, sf::Color::White);
        };


    sf::Vector2i* focusedVertex = nullptr;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::MouseButtonPressed)
            {
                sf::Vector2i position{ event.mouseButton.x, event.mouseButton.y };
                for (auto element : trianglePositions)
                {
                    if (Distance(*element, position) < 5.0f)
                    {
                        focusedVertex = element;
                    }
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased && focusedVertex)
            {
                focusedVertex = nullptr;
            }
            else if (event.type == sf::Event::MouseMoved)
            {
                if (focusedVertex != nullptr)
                {
                    focusedVertex->x = sf::Mouse::getPosition(window).x;
                    focusedVertex->y = sf::Mouse::getPosition(window).y;
                    ClearCanvas(canvasBuffer);
                    rast.DrawTriangle(triangle);
                    texture.loadFromImage(canvasBuffer);
                    mySprite.setTexture(texture);
                }
            }

            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }




        window.clear();
        window.draw(mySprite);

        for (const auto& p : trianglePositions)
        {
            sf::CircleShape s;
            s.setPosition(sf::Vector2f(*p));
            s.setRadius(2.0f);
            s.setFillColor(sf::Color::Black);
            window.draw(s);
        }

        window.display();
    }

    return 0;
}