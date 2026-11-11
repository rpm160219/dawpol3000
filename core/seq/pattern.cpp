#include "seq/pattern.h"

namespace dp {

int Pattern::find(int tick, int pitch) const {
    for (int i = 0; i < count; i++) {
        const Note& n = notes[i];
        if (n.pitch == pitch && tick >= n.start && tick < n.start + n.len) return i;
    }
    return -1;
}

int Pattern::maxLen(int start, int pitch, int ignore) const {
    int lim = lengthTicks;
    for (int i = 0; i < count; i++) {
        if (i == ignore || notes[i].pitch != pitch) continue;
        if (notes[i].start > start && notes[i].start < lim) lim = notes[i].start;
    }
    return lim - start;
}

int Pattern::add(Note n) {
    if (count >= MAX_NOTES || n.start >= lengthTicks) return -1;
    if (find(n.start, n.pitch) >= 0) return -1;
    int m = maxLen(n.start, n.pitch);
    if (n.len > m) n.len = (uint16_t)m;
    if (n.len == 0) return -1;
    notes[count] = n;
    return count++;
}

void Pattern::remove(int i) {
    if (i < 0 || i >= count) return;
    notes[i] = notes[--count];
}

}  // namespace dp
