/*
    Copyright (c) 2011, Gunther Piez <gpiez@web.de>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY Gunther Piez <gpiez@web.de> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL Gunther Piez <gpiez@web.de> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "sortedmovelist.h"
#include <cstring>

constexpr int rows[] = {111, 41, 13, 4, 1 };

void SortedMoveList::nodesCount(uint64_t n) {
    return;
    nodes[current] += n; }

void SortedMoveList::currentToFront() {
    OldMoveList::currentToFront();
    uint64_t temp = nodes[current];
    memmove(nodes+first+1, nodes+first, sizeof(uint64_t) * (current-first));
    nodes[first] = temp; }

void SortedMoveList::sort(unsigned bm) { //FIXME move bm into class
//         return;
    ASSERT(bm >= 1 && bm <= last);
//         bm = 1;

    for (unsigned k=0; k<5; k++) {
        unsigned h = rows[k];

        for (unsigned i = h+first+bm; i<last; i++) {
            Move tm = list[i];
            uint64_t tn = nodes[i];
            unsigned j=i;
            while (j>=h+first+bm && nodes[j-h]<tn) {
                list[j]=list[j-h];
                nodes[j]=nodes[j-h];
                j=j-h; }
            list[j]=tm;
            nodes[j]=tn; } }
    /*        for (int i=first; i<last; ++i) {
            std::cout << std::setw(2) << i-first << " " << list[i].algebraic() << ": " << std::setw(10) << nodes[i] << std::endl;
        }
        std::cout << std::endl;*/
}
