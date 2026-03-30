#pragma once
#include "ocf/math/RectBinPack.h"

namespace ocf {

class MaxRectsBinPack : public RectBinPack {
public:
    enum class FreeRectChoiceHeuristic {
        RectBestsShortSideFit,
        RectBestLongSideFit,
        RectBestAreaFit,
        RectBottomLeftRule,
        RectContactPointRule
    };


    MaxRectsBinPack();
    virtual ~MaxRectsBinPack();

    void init(float width, float height) override;

    void insert(std::vector<math::vec2> rects, std::vector<math::Rect>& dest) override;

    math::Rect insert(float width, float height) override;

    math::Rect scoreRect(float width, float height, float score1, float score2) const;

    void setHeuristic(FreeRectChoiceHeuristic heuristic) { m_heuristic = heuristic; }
    FreeRectChoiceHeuristic getHeuristic() const { return m_heuristic; }

    const std::vector<math::Rect>& getUsedRects() { return m_usedRects; }
    const std::vector<math::Rect>& getFreeRects() { return m_freeRects; }

private:
    math::Rect findPositionForShortSideFit(float width, float height,
                                     float& bestShortSideFit, float& bestLongSideFit) const;

    math::Rect findPositionForLongSideFit(float width, float height,
                                    float& bestShortSideFit, float& bestLongSideFit) const;

    math::Rect findPositionForAreaFit(float width, float height,
                                float& bestAreaFit, float& bestShortSideFit) const;

    math::Rect findPositionForNewNodeBottomLeft(float width, float height,
                                          float& bestY, float& bestX) const;

    math::Rect findPositionForContactPoint(float width, float height, float bestContactScore) const;

    float commonIntervalLength(float i1start, float i1end, float i2start, float i2end) const;

    float contactPointScoreNode(float x, float y, float width, float height) const;

    void placeRect(const math::Rect& node);

    bool splitFreeNode(const math::Rect& freeNode, const math::Rect& usedNode);

    void insertNewFreeRects(const math::Rect& newFreeRect);

    void pruneFreeList();

private:
    size_t m_newFreeRectsLastSize;
    std::vector<math::Rect> m_newFreeRects;
    std::vector<math::Rect> m_freeRects;
    std::vector<math::Rect> m_usedRects;
    FreeRectChoiceHeuristic m_heuristic;
};

} // namespace ocf