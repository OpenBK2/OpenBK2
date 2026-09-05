#pragma once

#include "Toolkit/sbarcore.h"
#include <algorithm>
#include <vector>

// Native CDockBar lays out toolbar buttons and preserves their pixel offsets.
// Pane rows instead fill the available edge, sharing it by weight. Keeping the
// native bar array retains MFC's drag insertion, floating placeholders and state.
class SECPaneDockBar : public CDockBar
{
public:
    CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz) override
    {
        bool hasPanes = false;
        for (int i = 0; i < m_arrBars.GetSize(); ++i)
        {
            if (dynamic_cast<SECControlBar*>(GetDockedControlBar(i)) != nullptr)
                hasPanes = true;
        }
        if (!hasPanes)
            return CDockBar::CalcFixedLayout(bStretch, bHorz);

        CRect available = m_rectLayout;
        if (available.IsRectEmpty())
            GetDockingFrame()->GetClientRect(&available);
        const int along = (std::max)(1, bHorz ? available.Width() : available.Height());
        const int across = (std::max)(1, bHorz ? available.Height() : available.Width());
        AFX_SIZEPARENTPARAMS layout = {};
        layout.hDWP = m_bLayoutQuery ? nullptr : ::BeginDeferWindowPos((int)m_arrBars.GetSize());
        std::vector<CControlBar*> row;
        int offset = 0;
        int usedAlong = 0;

        auto place = [&](CControlBar* bar, int pos, int length, int thickness)
        {
            CRect rect = bHorz ? CRect(pos, offset, pos + length, offset + thickness)
                              : CRect(offset, pos, offset + thickness, pos + length);
            if (!m_bLayoutQuery)
            {
                if (bar->m_pDockContext != nullptr)
                    bar->m_pDockContext->m_rectMRUDockPos = rect;
                AfxRepositionWindow(&layout, bar->GetSafeHwnd(), &rect);
                if (auto* pane = dynamic_cast<SECControlBar*>(bar))
                {
                    // Remember length for drag previews, retaining requested
                    // thickness when the main window is temporarily too small.
                    if (bHorz) pane->m_szDockHorz.cx = length;
                    else pane->m_szDockVert.cy = length;
                }
            }
        };

        auto layoutRow = [&]()
        {
            if (row.empty())
                return;
            int paneCount = 0;
            int fixedLength = 0;
            int thickness = 0;
            std::vector<int> lengths(row.size(), 0);
            std::vector<float> weights(row.size(), 0.0f);
            for (size_t i = 0; i < row.size(); ++i)
            {
                if (auto* pane = dynamic_cast<SECControlBar*>(row[i]))
                {
                    ++paneCount;
                    weights[i] = (std::max)(0.001f, pane->m_fDockedPctWidth);
                    thickness = (std::max)(thickness, (int)(bHorz ? pane->m_szDockHorz.cy : pane->m_szDockVert.cx));
                }
                else
                {
                    CSize size = row[i]->CalcDynamicLayout(-1, bHorz ? LM_HORZ | LM_HORZDOCK : LM_VERTDOCK);
                    lengths[i] = bHorz ? size.cx : size.cy;
                    fixedLength += lengths[i];
                    thickness = (std::max)(thickness, (int)(bHorz ? size.cy : size.cx));
                }
            }

            if (paneCount != 0)
            {
                // Reserve usable document space and fit even a very small frame.
                thickness = (std::max)(1, (std::min)(thickness, across - offset - 96));
                int remaining = (std::max)(paneCount, along - fixedLength);
                const int minimum = (std::min)(64, remaining / paneCount);
                float totalWeight = 0.0f;
                for (float weight : weights) totalWeight += weight;
                int pending = paneCount;
                // Fix undersized shares first, then divide the remaining pixels.
                bool clamped;
                do
                {
                    clamped = false;
                    for (size_t i = 0; i < row.size(); ++i)
                    {
                        if (weights[i] > 0 && remaining * weights[i] / totalWeight < minimum)
                        {
                            lengths[i] = minimum;
                            remaining -= minimum;
                            totalWeight -= weights[i];
                            weights[i] = 0;
                            --pending;
                            clamped = true;
                        }
                    }
                } while (clamped && pending > 0);
                for (size_t i = 0; i < row.size(); ++i)
                {
                    if (weights[i] <= 0) continue;
                    lengths[i] = pending == 1 ? remaining
                        : (int)(remaining * weights[i] / totalWeight);
                    remaining -= lengths[i];
                    totalWeight -= weights[i];
                    --pending;
                }
            }

            int pos = 0;
            for (size_t i = 0; i < row.size(); ++i)
            {
                // Toolbar rows can wrap in a narrow frame; pane rows never
                // acquire permanent extra columns merely because it shrank.
                if (paneCount == 0 && pos > 0 && pos + lengths[i] > along)
                {
                    usedAlong = (std::max)(usedAlong, pos);
                    pos = 0;
                    offset += thickness;
                }
                place(row[i], pos, lengths[i], thickness);
                pos += lengths[i];
            }
            usedAlong = (std::max)(usedAlong, pos);
            offset += thickness;
            row.clear();
        };

        for (int i = 0; i < m_arrBars.GetSize(); ++i)
        {
            if (m_arrBars[i] == nullptr)
                layoutRow();
            else if (CControlBar* bar = GetDockedControlBar(i))
            {
                if (bar->IsVisible()) row.push_back(bar);
            }
        }
        layoutRow();
        for (int i = 0; i < m_arrBars.GetSize(); ++i)
        {
            if (CControlBar* bar = GetDockedControlBar(i))
                bar->RecalcDelayShow(&layout);
        }
        if (layout.hDWP != nullptr)
            ::EndDeferWindowPos(layout.hDWP);

        CSize size = bHorz ? CSize(usedAlong, offset) : CSize(offset, usedAlong);
        CRect borders(0, 0, 0, 0);
        CalcInsideRect(borders, bHorz);
        if (size.cx > 0) size.cx += borders.left - borders.right;
        if (size.cy > 0) size.cy += borders.top - borders.bottom;
        if (bStretch)
        {
            if (bHorz) size.cx = 32767;
            else size.cy = 32767;
        }
        return size;
    }
};

namespace NDockEx
{
// Install pane-aware dock bars before MFC's EnableDocking fills missing edges
// and initializes its floating-frame class. The original ordering is significant.
inline void CreateDockBars(CFrameWnd* frame, DWORD style)
{
    const UINT ids[] = { AFX_IDW_DOCKBAR_TOP, AFX_IDW_DOCKBAR_BOTTOM,
                         AFX_IDW_DOCKBAR_LEFT, AFX_IDW_DOCKBAR_RIGHT };
    const DWORD styles[] = { CBRS_TOP, CBRS_BOTTOM, CBRS_LEFT, CBRS_RIGHT };
    for (int i = 0; i < 4; ++i)
    {
        if ((style & styles[i] & CBRS_ALIGN_ANY) == 0 || frame->GetControlBar(ids[i]) != nullptr)
            continue;
        auto* dock = new SECPaneDockBar;
        if (!dock->Create(frame, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | styles[i], ids[i]))
        {
            delete dock;
            AfxThrowResourceException();
        }
    }
}
}
