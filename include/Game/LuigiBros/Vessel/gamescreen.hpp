#pragma once
#include <cstddef>
#include <nn/gfx/gfx_Texture.h>
#include "common/aglGPUMemAddr.h"
#include "common/aglTextureData.h"
#include "common/aglVertexAttribute.h"
#include "common/aglVertexBuffer.h"

namespace agl { class ShaderProgram; }

class systemdrawscreen {
public:
    struct TextCallback;
};

class gamescreen {
public:
    enum MONITER : unsigned int { MonitorTV, MonitorDRC, MonitorBoth };
    enum GAMESCREEN : unsigned int { GameScreenFirst, GameScreenSecond, GameScreenBoth };
    enum FILTER : unsigned int { FilterNone };
    enum ALLOC_TYPE : unsigned int { AllocDefault };
    struct ScreenState {
        bool enabled;
        unsigned char _01[0x63];
    };
    struct MonitorState {
        ScreenState screens[2];
        bool enabled;
        unsigned char _c9[0xd7];
        nn::gfx::Texture texture;
        unsigned char _afterTexture[0xf8 - sizeof(nn::gfx::Texture)];
        void* _298 = nullptr;
        unsigned char _2a0[0x20];
        void* _2c0 = nullptr;
        FILTER filter;
        float strength;
        unsigned char _2d0[8];
    };
    struct GameState {
        FILTER gpuFilter;
        float gpuStrength;
        FILTER cpuFilter;
        float cpuStrength;
        unsigned char _10[8];
        agl::TextureData* pTexture;
        agl::GPUMemVoidAddr image;
        agl::TextureData* pOwnedTexture;
    };
    struct Vtx {
        /**
         * @brief Initializes a screen vertex and texture coordinates.
         * @param x Horizontal position.
         * @param y Vertical position.
         * @param u Horizontal texture coordinate.
         * @param v Vertical texture coordinate.
         */
        Vtx(float x, float y, float u, float v) : x(x), y(y), u(u), v(v) {}
        /**
         * @brief Copies a screen vertex's components.
         * @param rOther Source vertex.
         * @return This vertex.
         */
        Vtx& operator=(const Vtx& rOther) {
            x = rOther.x;
            y = rOther.y;
            u = rOther.u;
            v = rOther.v;
            return *this;
        }
        float x, y, u, v;
    };
    ~gamescreen();
    void init();
    void draw();
    void finalize();
    void screen(MONITER monitor, GAMESCREEN game, bool enabled);
    bool IsScreen(MONITER monitor, GAMESCREEN game);
    void gpuFilter(MONITER monitor, FILTER filter, float strength);
    void gpuFilter(GAMESCREEN game, FILTER filter, float strength);
    void cpuFilter(GAMESCREEN game, FILTER filter, float strength);
    void entryTextCallback(MONITER monitor, const systemdrawscreen::TextCallback& rCallback);
    void cancelTextCallback(MONITER monitor, const systemdrawscreen::TextCallback& rCallback);
    void m_HomeButtonMenuToDRC();
    void acquiredForeground();
    void releaseForeground();
    static void* malloc(ALLOC_TYPE type, unsigned int size, unsigned int alignment);
    void buildDevice();
    void buildRenderTargets();

    MonitorState mMonitors[2];
    GameState mGames[2];
    unsigned int mHeight;
    unsigned int mWidth;
    unsigned char _638[8];
    unsigned char* mpPixels;
    unsigned char _648[0xc];
    unsigned int mRefresh;
    unsigned char _658[0x3088];
    agl::ShaderProgram* mpShader;
    unsigned char _36e8[8];
    agl::GPUMemBlock<Vtx> mVertices;
    agl::VertexBuffer* mpVertexBuffer;
    agl::VertexAttribute mVertexAttribute;
};
static_assert(sizeof(gamescreen::ScreenState) == 0x64, "Screen state layout");
static_assert(sizeof(gamescreen::MonitorState) == 0x2d8, "Monitor state layout");
static_assert(offsetof(gamescreen, mGames) == 0x5b0, "Game filter layout");

static_assert(offsetof(gamescreen, mVertices) == 0x36f0, "Vertex storage layout");

extern gamescreen m_gamescreen;
