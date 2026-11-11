// SPDX-License-Identifier: MIT
#include "ocf/scene/animation/Tweener.h"

#include "ocf/scene/Node.h"

#include <algorithm>

namespace ocf {

Tweener::Tweener() = default;

Tweener::~Tweener() = default;

void Tweener::update(float deltaTime)
{
    if (m_paused || m_owner == nullptr) {
        return;
    }

    const float scaledTime = deltaTime * m_timeScale;

    // Callbacks may run tweens, which appends to m_entries, so the loop goes by index and the
    // count is taken first: tweens run during this update begin at the next one
    const size_t count = m_entries.size();
    for (size_t i = 0; i < count; ++i) {
        if (m_entries[i].stopped) {
            continue;
        }
        // The tween is moved out while it runs, since appending may reallocate m_entries
        TweenPtr tween = std::move(m_entries[i].tween);
        if (!m_entries[i].started) {
            m_entries[i].started = true;
            tween->start(*m_owner);
        }
        tween->update(*m_owner, scaledTime);
        if (tween->isFinished()) {
            m_entries[i].stopped = true;
        }
        m_entries[i].tween = std::move(tween);
    }

    std::erase_if(m_entries, [](const Entry& entry) { return entry.stopped; });
}

Tweener::TweenId Tweener::run(TweenPtr tween)
{
    if (tween == nullptr) {
        return 0;
    }
    const TweenId id = m_nextId++;
    if (m_nextId == 0) {
        m_nextId = 1;
    }
    m_entries.push_back(Entry{id, std::move(tween)});
    return id;
}

void Tweener::stop(TweenId id)
{
    // Only marks the entry, since this may be called from a callback during update()
    for (Entry& entry : m_entries) {
        if (entry.id == id) {
            entry.stopped = true;
        }
    }
}

void Tweener::stopAll()
{
    for (Entry& entry : m_entries) {
        entry.stopped = true;
    }
}

bool Tweener::isRunning(TweenId id) const
{
    return std::any_of(m_entries.begin(), m_entries.end(), [id](const Entry& entry) {
        return entry.id == id && !entry.stopped;
    });
}

bool Tweener::isRunning() const
{
    return std::any_of(m_entries.begin(), m_entries.end(),
                       [](const Entry& entry) { return !entry.stopped; });
}

} // namespace ocf
