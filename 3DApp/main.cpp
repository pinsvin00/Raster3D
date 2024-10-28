#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <iostream>
#include <algorithm>
#include <functional>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

constexpr int CANVAS_WIDTH = 1600;
constexpr int CANVAS_HEIGHT = 900;

struct Triangle3D {
    Triangle3D(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
        : mP1(p1), mP2(p2), mP3(p3)
    {
    }
    glm::vec3 mP1;
    glm::vec3 mP2;
    glm::vec3 mP3;
};

struct Triangle
{
    Triangle(sf::Vector3i p1, sf::Vector3i p2, sf::Vector3i p3)
        : mP1(p1), mP2(p2), mP3(p3)
    {
    }
    sf::Vector3i mP1;
    sf::Vector3i mP2;
    sf::Vector3i mP3;
};


class Rasterizer
{
private:
    sf::Image* mBitmap;
    bool mDebugColors;
    std::function<sf::Color(int, int)> mColorCb;

public:

    void DrawLine(int sx, int sy, int ex, sf::Color color)
    {
        if (sy >= 0 && sy < CANVAS_HEIGHT)
        {
            ex = std::min(static_cast<int>(mBitmap->getSize().x), ex);
            int cx = sx;
            for (; cx < ex && cx >= 0 && cx < CANVAS_WIDTH; cx++)
            {
                if (mBitmap->getPixel(cx, sy) != sf::Color::White)
                    mBitmap->setPixel(cx, sy, color);
            }
            if (cx >= 0 && cx < CANVAS_WIDTH)
                mBitmap->setPixel(cx, sy, sf::Color::White);
        }
    }

    void DrawLine(int sx, int sy, int ex)
    {
        DrawLine(sx, sy, ex, mColorCb(sx, sy));
    }

    inline static int LerpVer(const sf::Vector3i& A, const sf::Vector3i distance, int y)
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

        const sf::Vector3i longerSideDistance = t.mP1 - t.mP3;
        const sf::Vector3i shorterSideDistance = t.mP1 - t.mP2;
        const sf::Vector3i bottomSideDistance = t.mP2 - t.mP3;

        for (int i = t.mP1.y; i < t.mP2.y; i++)
        {
            int sx = LerpVer(t.mP1, longerSideDistance, i);
            int ex = LerpVer(t.mP2, shorterSideDistance, i);
            if (sx > ex)
                std::swap(sx, ex);

            DrawLine(sx, i, ex);

        }
        for (int i = t.mP2.y; i < t.mP3.y; i++)
        {
            int sx = LerpVer(t.mP1, longerSideDistance, i);
            int ex = LerpVer(t.mP2, bottomSideDistance, i);
            if (sx > ex)
                std::swap(sx, ex);

            if(i == t.mP3.y - 1)
                DrawLine(sx, i, ex, sf::Color::White);
            else
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


class Cube {
public:
    Triangle3D triangles[12] = {
        Triangle3D(glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.5, 0.5, 0.5)),
        Triangle3D(glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, 0.5)),

        // Back face
        Triangle3D(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, -0.5)),
        Triangle3D(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, -0.5)),

        // Left face
        Triangle3D(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5)),
        Triangle3D(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5)),

        // Right face
        Triangle3D(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, 0.5)),
        Triangle3D(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.5, -0.5, 0.5)),

        // Top face
        Triangle3D(glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, 0.5)),
        Triangle3D(glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, -0.5)),

        // Bottom face
        Triangle3D(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, 0.5)),
        Triangle3D(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, 0.5), glm::vec3(-0.5, -0.5, 0.5))
    };
};

int main()
{
    //set framerate limit
    sf::RenderWindow window(sf::VideoMode(CANVAS_WIDTH, CANVAS_HEIGHT), "Basic renderer test");
    sf::Image canvasBuffer;
    sf::Texture texture;
    sf::Sprite mySprite;
    Rasterizer rast(&canvasBuffer, [&](int x, int y) {
        return sf::Color::Red;
    });
    Cube c;
    sf::Clock clk;

    canvasBuffer.create(CANVAS_WIDTH, CANVAS_HEIGHT, sf::Color::Black);
    texture.loadFromImage(canvasBuffer);
    window.setFramerateLimit(20);
    mySprite.setTexture(texture);

    auto ClearCanvas = [](sf::Image& canvas)
        {
            canvas.create(CANVAS_WIDTH, CANVAS_HEIGHT, sf::Color::Black);
        };

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }

        window.clear();
        ClearCanvas(canvasBuffer);

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.1f));
        model = glm::translate(model, glm::vec3(0.1f * glm::sin(clk.getElapsedTime().asSeconds() / 2.0f), 0, 0));
        model = glm::rotate(model, 6.28f * glm::sin(clk.getElapsedTime().asSeconds() / 2.0f), glm::vec3(1.0f));

        for (auto& element : c.triangles)
        {
            glm::vec3 A = proj * model * glm::vec4(element.mP1, 1.0f);
            glm::vec3 B = proj * model * glm::vec4(element.mP2, 1.0f);
            glm::vec3 C = proj * model * glm::vec4(element.mP3, 1.0f);

            sf::Vector3i transformedA = sf::Vector3i((A.x + 0.5f) * CANVAS_WIDTH, (A.y + 0.5f) * CANVAS_HEIGHT, A.z);
            sf::Vector3i transformedB = sf::Vector3i((B.x + 0.5f) * CANVAS_WIDTH, (B.y + 0.5f) * CANVAS_HEIGHT, B.z);
            sf::Vector3i transformedC = sf::Vector3i((C.x + 0.5f) * CANVAS_WIDTH, (C.y + 0.5f) * CANVAS_HEIGHT, C.z);

            Triangle t = { transformedA, transformedB, transformedC };
            rast.DrawTriangle(t);
        }

        texture.loadFromImage(canvasBuffer);
        mySprite.setTexture(texture);

        window.draw(mySprite);
        window.display();
    }

    return 0;
}