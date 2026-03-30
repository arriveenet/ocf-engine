#include "ocf/math/MaxRectsBinPack.h"
#include "ocf/base/Macros.h"
#include <cmath>
#include <limits>

namespace ocf {

using namespace math;

MaxRectsBinPack::MaxRectsBinPack()
    : m_newFreeRectsLastSize(0)
    , m_heuristic(FreeRectChoiceHeuristic::RectBestAreaFit)
{
}

MaxRectsBinPack::~MaxRectsBinPack()
{
}

void MaxRectsBinPack::init(float width, float height)
{
    m_binWidth = width;
    m_binHeight = height;

    Rect n;
    n.m_position = vec2(0, 0);
    n.m_size = vec2(width, height);

    m_usedRects.clear();
    m_freeRects.clear();

    m_freeRects.emplace_back(n);
}

void MaxRectsBinPack::insert(std::vector<vec2> rects, std::vector<Rect>& dest)
{
    dest.clear();
    dest.reserve(rects.size());

    while (!rects.empty()) {
        float bestScore1 = std::numeric_limits<float>::max();
        float bestScore2 = std::numeric_limits<float>::max();
        int bestRectIndex = -1;
        Rect bestNode;

        int rectIndex = 0;
        for (const auto& rect : rects) {
            float score1 = 0.0f, score2 = 0.0f;
            Rect newNode = scoreRect(rect.x, rect.y, score1, score2);

            if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2)) {
                bestScore1 = score1;
                bestScore2 = score2;
                bestNode = newNode;
                bestRectIndex = rectIndex;
            }

            rectIndex++;
        }

        if (bestRectIndex == -1)
            return;

        placeRect(bestNode);
        dest.push_back(bestNode);
        rects[bestRectIndex] = rects.back();
        rects.pop_back();
    }
}

Rect MaxRectsBinPack::insert(float width, float height)
{
    Rect newNode;

    float score1 = std::numeric_limits<float>::max();
    float score2 = std::numeric_limits<float>::max();

    switch (m_heuristic) {
    case MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestsShortSideFit:
        newNode = findPositionForShortSideFit(width, height, score1, score2);
        break;
    case MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestLongSideFit:
        newNode = findPositionForLongSideFit(width, height, score1, score2);
        break;
    case MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestAreaFit:
        newNode = findPositionForAreaFit(width, height, score1, score2);
        break;
    case MaxRectsBinPack::FreeRectChoiceHeuristic::RectBottomLeftRule:
        newNode = findPositionForNewNodeBottomLeft(width, height, score1, score2);
        break;
    default:
        OCFASSERT(false, "Invalid heuristic");
        break;
    }

    if (newNode.m_size.y == 0)
        return newNode;

    placeRect(newNode);

    return newNode;
}

Rect MaxRectsBinPack::scoreRect(float width, float height, float score1, float score2) const
{
    Rect newNode;
    score1 = std::numeric_limits<float>::max();
    score2 = std::numeric_limits<float>::max();

    switch (m_heuristic) {
    case MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestsShortSideFit:
        newNode = findPositionForShortSideFit(width, height, score1, score2);
        break;
    case MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestLongSideFit:
        newNode = findPositionForLongSideFit(width, height, score1, score2);
        break;
    case MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestAreaFit:
        newNode = findPositionForAreaFit(width, height, score1, score2);
        break;
    case MaxRectsBinPack::FreeRectChoiceHeuristic::RectBottomLeftRule:
        newNode = findPositionForNewNodeBottomLeft(width, height, score1, score2);
        break;
    case MaxRectsBinPack::FreeRectChoiceHeuristic::RectContactPointRule:
        newNode = findPositionForContactPoint(width, height, score1);
        score1 = -score1;
        break;
    default:
        OCFASSERT(false, "Invalid heuristic");
        break;
    }

    if (newNode.m_size.y == 0) {
        score1 = std::numeric_limits<float>::max();
        score2 = std::numeric_limits<float>::max();
    }

    return newNode;
}

Rect MaxRectsBinPack::findPositionForShortSideFit(float width, float height,
                                                  float& bestShortSideFit,
                                                  float& bestLongSideFit) const
{
    Rect bestNode;

    bestShortSideFit = std::numeric_limits<float>::max();
    bestLongSideFit = std::numeric_limits<float>::max();

    for (const auto& freeRect : m_freeRects) {

        if (freeRect.m_size.x >= width && freeRect.m_size.y >= height) {
            const float leftoverHoriz = std::abs(freeRect.m_size.x - width);
            const float leftoverVert = std::abs(freeRect.m_size.y - height);
            const float shortSideFit = std::min(leftoverHoriz, leftoverVert);
            const float longSideFit = std::max(leftoverHoriz, leftoverVert);

            if (shortSideFit < bestShortSideFit 
                || (shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit)) {
                bestNode.m_position.x = freeRect.m_position.x;
                bestNode.m_position.y = freeRect.m_position.y;
                bestNode.m_size.x = width;
                bestNode.m_size.y = height;
                bestShortSideFit = shortSideFit;
                bestLongSideFit = longSideFit;
            }
        }
    }

    return bestNode;
}

Rect MaxRectsBinPack::findPositionForLongSideFit(float width, float height,
                                                 float& bestShortSideFit,
                                                 float& bestLongSideFit) const
{
    Rect bestNode;

    bestShortSideFit = std::numeric_limits<float>::max();
    bestLongSideFit = std::numeric_limits<float>::max();

    for (const auto& freeRect : m_freeRects) {

        if (freeRect.m_size.x >= width && freeRect.m_size.y >= height) {
            const float leftoverHoriz = std::abs(freeRect.m_size.x - width);
            const float leftoverVert = std::abs(freeRect.m_size.y - height);
            const float shortSideFit = std::min(leftoverHoriz, leftoverVert);
            const float longSideFit = std::max(leftoverHoriz, leftoverVert);

            if (longSideFit < bestLongSideFit
                || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit)) {
                bestNode.m_position.x = freeRect.m_position.x;
                bestNode.m_position.y = freeRect.m_position.y;
                bestNode.m_size.x = width;
                bestNode.m_size.y = height;
                bestShortSideFit = shortSideFit;
                bestLongSideFit = longSideFit;
            }
        }
    }

    return bestNode;
}

Rect MaxRectsBinPack::findPositionForAreaFit(float width, float height,
                                             float& bestAreaFit,
                                             float& bestShortSideFit) const
{
    Rect bestNode;

    bestAreaFit = std::numeric_limits<float>::max();
    bestShortSideFit = std::numeric_limits<float>::max();

    for (const auto& freeRect : m_freeRects) {

        const float areaFit = (freeRect.m_size.x * freeRect.m_size.y) - (width * height);

        if (freeRect.m_size.x >= width && freeRect.m_size.y >= height) {
            const float leftoverHoriz = std::abs(freeRect.m_size.x - width);
            const float leftoverVert = std::abs(freeRect.m_size.y - height);
            const float shortSideFit = std::min(leftoverHoriz, leftoverVert);

            if (areaFit < bestAreaFit 
                || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit)) {
                bestNode.m_position.x = freeRect.m_position.x;
                bestNode.m_position.y = freeRect.m_position.y;
                bestNode.m_size.x = width;
                bestNode.m_size.y = height;
                bestAreaFit = areaFit;
                bestShortSideFit = shortSideFit;
            }
        }
    }

    return bestNode;
}

Rect MaxRectsBinPack::findPositionForNewNodeBottomLeft(float width, float height,
                                                       float& bestY, float& bestX) const
{
    Rect bestNode;

    bestY = std::numeric_limits<float>::max();
    bestX = std::numeric_limits<float>::max();

    for (const auto& freeRect : m_freeRects) {

        if (freeRect.m_size.x >= width && freeRect.m_size.y >= height) {
            const float topSideY = freeRect.m_position.y + height;

            if (topSideY < bestY || (topSideY == bestY && freeRect.m_position.x < bestX)) {
                bestNode.m_position.x = freeRect.m_position.x;
                bestNode.m_position.y = freeRect.m_position.y;
                bestNode.m_size.x = width;
                bestNode.m_size.y = height;
                bestY = topSideY;
                bestX = freeRect.m_position.x;
            }
        }
    }

    return bestNode;
}

Rect MaxRectsBinPack::findPositionForContactPoint(float width, float height,
                                                  float bestContactScore) const
{
    Rect bestNode;

    bestContactScore = -1.0f;

    for (const auto& freeRect : m_freeRects) {
        if (freeRect.m_size.x >= width && freeRect.m_size.y >= height) {
            const float score = contactPointScoreNode(freeRect.m_position.x,
                                                      freeRect.m_position.y,
                                                      width, height);

            if (score > bestContactScore) {
                bestNode.m_position.x = freeRect.m_position.x;
                bestNode.m_position.y = freeRect.m_position.y;
                bestNode.m_size.x = width;
                bestNode.m_size.y = height;
                bestContactScore = score;
            }
        }
    }

    return bestNode;
}

float MaxRectsBinPack::commonIntervalLength(float i1start, float i1end,
                                            float i2start, float i2end) const
{
    if (i1end < i2start || i2end < i1start)
        return 0.0f;

    return std::min(i1end, i2end) - std::max(i1start, i2start);
}

float MaxRectsBinPack::contactPointScoreNode(float x, float y, float width, float height) const
{
    float score = 0.0f;

    if (x == 0 || x + width == m_binWidth)
        score += height;

    if (y == 0 || y + height == m_binHeight)
        score += width;

    for (const auto& usedRect : m_usedRects) {

        if ((usedRect.m_position.x == x + width)
            || (usedRect.m_position.x + usedRect.m_size.x == x)) {
            score += commonIntervalLength(usedRect.m_position.y, usedRect.m_position.y + usedRect.m_size.y, y, y + height);
        }

        if ((usedRect.m_position.y == y + height)
            || (usedRect.m_position.y + usedRect.m_size.y == y)) {
            score += commonIntervalLength(usedRect.m_position.x, usedRect.m_position.x + usedRect.m_size.x, x, x + width);
        }
    }

    return score;
}

void MaxRectsBinPack::placeRect(const Rect& node)
{
    for (size_t i = 0; i < m_freeRects.size();) {

        if (splitFreeNode(m_freeRects[i], node)) {
            m_freeRects[i] = m_freeRects.back();
            m_freeRects.pop_back();
        }
        else {
            ++i;
        }
    }

    pruneFreeList();

    m_usedRects.emplace_back(node);
}

bool MaxRectsBinPack::splitFreeNode(const Rect& freeNode, const Rect& usedNode)
{
    if (!freeNode.intersect(usedNode))
        return false;

    m_newFreeRectsLastSize = m_newFreeRects.size();

    if (usedNode.getMinX() < freeNode.getMaxX() && usedNode.getMaxX() > freeNode.getMinX()) {
        // 使用済みノードの上側にある新しい空きノード
        if (usedNode.getMinY() > freeNode.getMinY() && usedNode.getMinY() < freeNode.getMaxY()) {
            Rect newNode = freeNode;
            newNode.m_size.y = usedNode.m_position.y - newNode.m_position.y;
            insertNewFreeRects(newNode);
        }

        // 使用済みノードの下側にある新しい空きノード
        if (usedNode.getMaxY() < freeNode.getMaxY()) {
            Rect newNode = freeNode;
            newNode.m_position.y = usedNode.getMaxY();
            newNode.m_size.y = freeNode.getMaxY() - usedNode.getMaxY();
            insertNewFreeRects(newNode);
        }
    }

    if (usedNode.getMinY() < freeNode.getMaxX() && usedNode.getMaxY() > freeNode.getMinY()) {
        // 使用済みノードの左側にある新しい空きノード
        if (usedNode.getMinX() > freeNode.getMinX() && usedNode.getMinX() < freeNode.getMaxX()) {
            Rect newNode = freeNode;
            newNode.m_size.x = usedNode.m_position.x - newNode.m_position.x;
            insertNewFreeRects(newNode);
        }

        // 使用済みノードの右側にある新しい空きノード
        if (usedNode.getMaxX() < freeNode.getMaxX()) {
            Rect newNode = freeNode;
            newNode.m_position.x = usedNode.getMaxX();
            newNode.m_size.x = freeNode.getMaxX() - usedNode.getMaxX();
            insertNewFreeRects(newNode);
        }
    }

    return true;
}

void MaxRectsBinPack::insertNewFreeRects(const Rect& newFreeRect)
{
    OCF_ASSERT(newFreeRect.m_size.x > 0.0f);
    OCF_ASSERT(newFreeRect.m_size.y > 0.0f);

    for (size_t i = 0; i < m_newFreeRectsLastSize;) {

        if (m_newFreeRects[i].contain(newFreeRect))
            return;

        if (newFreeRect.contain(m_newFreeRects[i])) {
            m_newFreeRects[i] = m_newFreeRects[--m_newFreeRectsLastSize];
            m_newFreeRects[m_newFreeRectsLastSize] = m_newFreeRects.back();
            m_newFreeRects.pop_back();
        }
        else {
            i++;
        }
    }

    m_newFreeRects.emplace_back(newFreeRect);
}

void MaxRectsBinPack::pruneFreeList()
{
    for (size_t i = 0; i < m_freeRects.size(); i++) {
        for (size_t j = 0; j < m_newFreeRects.size();) {
            if (m_newFreeRects[j].contain(m_freeRects[i])) {
                m_newFreeRects[j] = m_newFreeRects.back();
                m_newFreeRects.pop_back();
            }
            else {
                j++;
            }
        }
    }

    m_freeRects.insert(m_freeRects.end(), m_newFreeRects.begin(), m_newFreeRects.end());
    m_newFreeRects.clear();
}

} // namespace ocf
