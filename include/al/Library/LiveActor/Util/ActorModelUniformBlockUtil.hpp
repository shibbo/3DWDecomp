#pragma once
namespace agl { class UniformBlock; }
namespace al {
class LiveActor;
struct UniformBlockLayout;
agl::UniformBlock* tryCreateModelUniformBlock(const LiveActor*, const char*, const UniformBlockLayout*, int);
agl::UniformBlock* findModelUniformBlock(const LiveActor*, const char*);
void swapModelUniformBlock(agl::UniformBlock*);
void flushModelUniformBlock(agl::UniformBlock*);
}
