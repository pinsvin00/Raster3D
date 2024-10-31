#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <iostream>
#include <algorithm>
#include <functional>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <algorithm>

constexpr int CANVAS_WIDTH = 400;
constexpr int CANVAS_HEIGHT = 300;

using Vector3 = glm::vec3;

struct Triangle {
    Triangle(Vector3 p1, Vector3 p2, Vector3 p3)
        : mP1(p1), mP2(p2), mP3(p3)
    {
    }
    Vector3 mP1;
    Vector3 mP2;
    Vector3 mP3;
};

class Rasterizer
{
private:
    sf::Image* mCanvas;
    Triangle* currentTriangle = nullptr;
    float* zDepthBuffer;
    std::function<sf::Color(const Triangle*, const glm::vec3)> mColorCb;

public:

    sf::Color colorOverride;

    float LerpZ(int startY, int endY, int currentY, float startZ, float endZ) {
        float t = (currentY - startY) / static_cast<float>(endY - startY);
        return startZ + (endZ - startZ) * t;
    }

    void SetPixel(int x, int y, float zDepth, sf::Color color)
    {
        if (zDepthBuffer[x + y * CANVAS_WIDTH] > zDepth)
        {
            zDepthBuffer[x + y * CANVAS_WIDTH] = zDepth;
            mCanvas->setPixel(x, y, color);
        }
    }

    void DrawLine(int sx, int sy, int ex, float sz, float ez, sf::Color color)
    {
        if (sy < 0 || sy >= CANVAS_HEIGHT) return;

        sx = std::max(0, std::min(sx, CANVAS_WIDTH - 1));
        ex = std::max(0, std::min(ex, CANVAS_WIDTH - 1));

        if (sx > ex) {
            std::swap(sx, ex);
            std::swap(sz, ez);
        }

        for (int cx = sx; cx <= ex; ++cx) {
            float z = LerpZ(sx, ex, cx, sz, ez);
            SetPixel(cx, sy, z, colorOverride); 
        }
    }

    void DrawLine(int sx, int sy, int ex, float sz, float ez)
    {
        DrawLine(sx, sy, ex, sz, ez, sf::Color::Red);
    }

    inline static int Lerp(const Vector3 A, const Vector3 distance, int y)
    {
        int dis = y - A.y;
        return A.x + (distance.x) * dis / distance.y;
    }

    Triangle NDCTriangle(Triangle tWS)
    {
        Vector3 ndcA = Vector3(
            static_cast<int>((tWS.mP1.x + 1.0f) * 0.5f * CANVAS_WIDTH),
            static_cast<int>((tWS.mP1.y + 1.0f) * 0.5f * CANVAS_HEIGHT),
            tWS.mP1.z
        );
        Vector3 ndcB = Vector3(
            static_cast<int>((tWS.mP2.x + 1.0f) * 0.5f * CANVAS_WIDTH),
            static_cast<int>((tWS.mP2.y + 1.0f) * 0.5f * CANVAS_HEIGHT),
            tWS.mP2.z
        );
        Vector3 ndcC = Vector3(
            static_cast<int>((tWS.mP3.x + 1.0f) * 0.5f * CANVAS_WIDTH),
            static_cast<int>((tWS.mP3.y + 1.0f) * 0.5f * CANVAS_HEIGHT),
            tWS.mP3.z
        );
        Triangle tNDC = { ndcA, ndcB, ndcC };
        return tNDC;
    }

    void DrawTriangle(Triangle tWS)
    {
        Triangle tNDC = NDCTriangle(tWS);
        //sort so we got the points on the top, as p1..p2, 
        if (tNDC.mP2.y > tNDC.mP3.y) std::swap(tNDC.mP2, tNDC.mP3);
        if (tNDC.mP1.y > tNDC.mP2.y) std::swap(tNDC.mP1, tNDC.mP2);
        if (tNDC.mP2.y > tNDC.mP3.y) std::swap(tNDC.mP2, tNDC.mP3);

        //longerSide = tNDC.mP1 -> tNDC.mP3;
        //shorterSide = tNDC.mP1 -> tNDC.mP2;
        //bottomSide = tNDC.mP2 -> tNDC.mP3;

        for (int i = tNDC.mP1.y; i < tNDC.mP2.y; i++)
        {
            int sx = Lerp(tNDC.mP1, tNDC.mP1 - tNDC.mP3, i);
            int ex = Lerp(tNDC.mP2, tNDC.mP1 - tNDC.mP2, i);
            float sz = LerpZ(tNDC.mP1.y, tNDC.mP3.y, i, tNDC.mP1.z, tNDC.mP3.z);
            float ez = LerpZ(tNDC.mP1.y, tNDC.mP2.y, i, tNDC.mP1.z, tNDC.mP2.z);
            if (sx > ex)
            {
                std::swap(sx, ex);
                std::swap(sz, ez);
            }
            DrawLine(sx, i, ex, sz, ez);
        }
        for (int i = tNDC.mP2.y; i < tNDC.mP3.y; i++)
        {
            int sx = Lerp(tNDC.mP1, tNDC.mP1 - tNDC.mP3, i);
            int ex = Lerp(tNDC.mP2, tNDC.mP2 - tNDC.mP3, i);
            float sz = LerpZ(tNDC.mP1.y, tNDC.mP3.y, i, tNDC.mP1.z, tNDC.mP3.z);
            float ez = LerpZ(tNDC.mP2.y, tNDC.mP3.y, i, tNDC.mP2.z, tNDC.mP3.z);
            if (sx > ex)
            {
                std::swap(sx, ex);
                std::swap(sz, ez);
            }
            DrawLine(sx, i, ex, sz, ez);
        }
    }

    void Clear()
    {
        for (size_t i = 0; i < CANVAS_WIDTH * CANVAS_HEIGHT; i++)
            zDepthBuffer[i] = 999.0f;
        mCanvas->create(CANVAS_WIDTH, CANVAS_HEIGHT, sf::Color::Black);
    }

    Rasterizer(
        sf::Image* canvas,
        std::function<sf::Color(const Triangle*, const glm::vec3)> colorCb = [](const Triangle*, const glm::vec3){ return sf::Color::Green; },
        bool useDebugColors = false
       ) : 
        mCanvas(canvas),
        mColorCb(colorCb)
    {
        zDepthBuffer = new float[CANVAS_WIDTH * CANVAS_HEIGHT];
    }
    ~Rasterizer()
    {
        delete zDepthBuffer;
    }
};


class Cube {
public:
    Triangle triangles[12] = {
        Triangle(glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.5, 0.5, 0.5)),
        Triangle(glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, 0.5)),

        // Back face
        Triangle(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, -0.5)),
        Triangle(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, -0.5)),

        // Left face
        Triangle(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5)),
        Triangle(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5)),

        // Right face
        Triangle(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, 0.5)),
        Triangle(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.5, -0.5, 0.5)),

        // Top face
        Triangle(glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, 0.5)),
        Triangle(glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, -0.5)),

        // Bottom face
        Triangle(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, 0.5)),
        Triangle(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, 0.5), glm::vec3(-0.5, -0.5, 0.5))
    };
};

sf::Color ShaderFunction(int x, int y, float depth)
{
    return sf::Color::White;
}

int main()
{
    //set framerate limit
    sf::RenderWindow window(sf::VideoMode(CANVAS_WIDTH, CANVAS_HEIGHT), "Basic renderer test");
    sf::Image canvasBuffer;
    sf::Texture texture;
    sf::Sprite mySprite;
    Rasterizer rast(&canvasBuffer, [&](const Triangle* triangle, glm::vec3 pos) {
        return sf::Color::Red;
    });
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT, 0.1f, 100.0f);;

    Cube c;
    sf::Clock clk;

    canvasBuffer.create(CANVAS_WIDTH, CANVAS_HEIGHT, sf::Color::Black);
    texture.loadFromImage(canvasBuffer);
    window.setFramerateLimit(20);
    mySprite.setTexture(texture);

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
        rast.Clear();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.5f));
        model = glm::rotate(model, 6.28f * glm::sin(clk.getElapsedTime().asSeconds() / 2.0f), glm::vec3(1.0f, 1.0f, 1.0f));

        int i = 0;
        std::vector<sf::Color> colors = {
            sf::Color::Red, sf::Color::Red,
            sf::Color::Green, sf::Color::Green,
            sf::Color::White, sf::Color::White,
            sf::Color::Yellow, sf::Color::Yellow,
            sf::Color::Magenta, sf::Color::Magenta,
            sf::Color::Cyan, sf::Color::Cyan,
        };

        for (auto& element : c.triangles)
        {
            glm::vec3 A = projection * model * glm::vec4(element.mP1, 1.0f);
            glm::vec3 B = projection * model * glm::vec4(element.mP2, 1.0f);
            glm::vec3 C = projection * model * glm::vec4(element.mP3, 1.0f);
            Triangle t = { A, B, C };
            rast.colorOverride = colors[i];
            rast.DrawTriangle(t);
            i++;
        }

        texture.loadFromImage(canvasBuffer);
        mySprite.setTexture(texture);

        window.draw(mySprite);
        window.display();
    }

    return 0;
}