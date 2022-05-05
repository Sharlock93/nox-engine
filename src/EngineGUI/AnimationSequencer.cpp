#include <EngineGUI/AnimationSequencer.h>

AnimationSequencer::AnimationSequencer(NoxEngine::AnimationComponent *animComp, NoxEngine::GameState* game_state) :
mFrameMin(0), 
mFrameMax(animComp->numTicks[animComp->animationIndex] - 1),
animComp(animComp),
game_state(game_state)
{
   
}

AnimationSequencer::AnimationSequencer() :mFrameMin(0), mFrameMax(0) {};

int AnimationSequencer::GetItemTypeCount() const 
{
    return (i32) game_state->audioSources.size();
}

const char* AnimationSequencer::GetItemTypeName(int index) const
{
    if (!game_state->audioSources.empty())
    {
        //return game_state->audioSources.
        auto startItr = game_state->audioSources.begin();
        auto endItr = game_state->audioSources.end();
        for (u32 i = 0; i < index; i++)
        {
            startItr++;
        }

        return startItr->second.name.c_str();
    }

    return "hahaha";
}

const char* AnimationSequencer::GetItemLabel(int index) const
{
    //return "Animation"; 
    //if (index > scene->allNodes.size() - 1)
    //{
    //    u32 audioIndex = index - scene->allNodes.size();
    //    if (!game_state->audioSources.empty())
    //    {
    //        return game_state->selectedAudio[audioIndex].c_str();
    //    }
    //    return "hahaha";
    //}

    //return scene->allNodes[index]->name.c_str();
    return "";
}

void AnimationSequencer::Get(int index, int** start, int** end, int* type, unsigned int* color)
{
    // Every animation is starting at 0
    static i32 items[] = { 0 };

    // I don't know why this code didn't work
    //i32 items2[] = { (scene->allNodes[index]->hasAnimations() ? scene->numTicks[scene->animationIndex] - 1 : 1) };

    if (color)
        *color = 0xFFAA8080; // same color for everyone, return color based on type
    if (start)
        *start = items;
    if (end)
    {
        *end = &animComp->numTicks[animComp->animationIndex];
    }
    if (type)
    {
        *type = index;
    }
}

size_t AnimationSequencer::GetCustomHeight(int index)
{
    return 0;
    // Every slot has to be the same size otherwise the bar will not be displayed
    //if (scene->allNodes[index]->hasAnimations())
    //    return 300;

    //return 0; 
}

void AnimationSequencer::CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect,
    const ImRect& clippingRect, const ImRect& legendClippingRect)
{
    //static const char* labels[] = { "Translation", "Rotation", "Scale" };

    //bool hasAnimation = scene->allNodes[index]->hasAnimations();

    //if (hasAnimation)
    //{
    //    draw_list->PushClipRect(legendRect.Min, legendRect.Max, true);
    //    for (int i = 0; i < 3; i++)
    //    {
    //        ImVec2 pta(legendRect.Min.x + 60, legendRect.Min.y + i * 20.0f);
    //        // Label
    //        draw_list->AddText(pta,
    //            scene->allNodes[index]->hasAnimations() ? 0xFFFFFFFF : 0x80FFFFFF, labels[i]);
    //    }
    //    draw_list->PopClipRect();

    //    draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
    //    for (int i = 0; i < 3; i++)
    //    {
    //        // The bar
    //        draw_list->AddRectFilled(ImVec2(rc.Min.x - 1, (rc.Min.y + 2) + i * 20.0f),
    //            ImVec2(rc.Max.x - 1, (rc.Min.y - 2) + (i + 1) * 20.0f), 0xFF80FF80, 4);
    //    }
    //    draw_list->PopClipRect();
    //}
}
