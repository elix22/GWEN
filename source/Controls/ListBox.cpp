//
// GWEN
// Copyright (c) 2013-2014 James Lammlein
// Copyright (c) 2010 Facepunch Studios
//
// This file is part of GWEN.
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Gwen.h"
#include "Gwen/Controls/ListBox.h"

#include "Gwen/Controls/Layout/Table.h"
#include "Gwen/Controls/Layout/TableRow.h"
#include "Gwen/Controls/ListBoxRow.h"
#include "Gwen/Controls/ScrollControl.h"
#include "Gwen/Controls/VerticalScrollBar.h"
#include "Gwen/input/gwen_input_base.h"

#include "Gwen/Utility.h"

namespace Gwen
{
namespace Controls
{

GWEN_CONTROL_CONSTRUCTOR(ListBox, ScrollControl)
{
    _multi_select = false;

    _inner_panel->SetPadding(Padding(2, 2, 2, 2));

    _table = new Controls::Layout::Table(this);
    _table->SetColumnCount(1);

    SetMargin(Margin(1, 1, 1, 1));
    SetPadding(Padding(0, 1, 0, 1));
}

Layout::TableRow* ListBox::AddItem(const std::string& label, const std::string& name)
{
    // Allocate a new row.
    ListBoxRow* row = new ListBoxRow(this);
    row->SetCellText(0, label);
    row->SetName(name);
    row->_on_selected.Add(this, &ListBox::OnRowSelected);
    row->_on_mouse_double_click_left.Add(this, &ListBox::OnRowMouseDoubleClickLeft);

    // Store the row.
    _table->AddRow(row, Position::TOP);

    return row;
}

void ListBox::RemoveItem(Layout::TableRow* row)
{
    _selected_rows.erase(std::find(_selected_rows.begin(), _selected_rows.end(), row));
    _table->Remove(row);
}

void ListBox::Clear()
{
    UnselectAll();
    _table->Clear();
}

void ListBox::SetAllowMultiSelect(bool multi_select)
{
    _multi_select = multi_select;
}

bool ListBox::GetAllowMultiSelect() const
{
    return _multi_select;
}

void ListBox::UnselectAll()
{
    std::list<Layout::TableRow*>::iterator i = _selected_rows.begin();
    while (i != _selected_rows.end())
    {
        ListBoxRow* row = dynamic_cast<ListBoxRow*>(*i);
        i = _selected_rows.erase(i);
        row->SetSelected(false);
    }
}

void ListBox::SetSelectedRow(Gwen::Controls::Base* row, bool clear_others)
{
    if (clear_others)
    {
        UnselectAll();
    }

    ListBoxRow* list_box_row = dynamic_cast<ListBoxRow*>(row);
    if (!list_box_row)
    {
        return;
    }

    list_box_row->SetSelected(true);
    _selected_rows.push_back(list_box_row);
    _on_row_selected.Call(this);
}

void ListBox::SelectByString(const std::string& string, bool clear_others)
{
    if (clear_others)
    {
        UnselectAll();
    }

    std::list<Base*>& children = _table->GetChildren();
    for (auto i = children.begin(); i != children.end(); ++i)
    {
        ListBoxRow* child = dynamic_cast<ListBoxRow*>(*i);
        if (!child)
        {
            continue;
        }

        if (Utility::String::Wildcard(string, child->GetCellText(0)))
        {
            SetSelectedRow(child, false);
        }
    }
}

Layout::TableRow* ListBox::GetSelectedRow()
{
    if (_selected_rows.empty())
    {
        return nullptr;
    }

    return *_selected_rows.begin();
}

const std::list<Layout::TableRow*>& ListBox::GetSelectedRows()
{
    return _selected_rows;
}

std::string ListBox::GetSelectedRowName()
{
    Layout::TableRow* row = GetSelectedRow();
    if (!row)
    {
        return "";
    }
    return row->GetName();
}

void ListBox::SetColumnCount(int count)
{
    _table->SetColumnCount(count);
}

void ListBox::SetColumnWidth(int count, int size)
{
    _table->SetColumnWidth(count, size);
}

void ListBox::Render(Skin::Base* skin)
{
    // Draw the list box.
    skin->DrawListBox(this);

    // Update the clipping region for the children.
    Gwen::Point point = LocalPositionToCanvas(Gwen::Point(0, 0));
    Gwen::Padding padding = GetPadding();

    Gwen::Rectangle bounds = GetBounds();
    bounds._x = point._x + padding._left;
    bounds._y = point._y + padding._top;
    bounds._width -= padding._left + padding._right;
    bounds._height -= padding._top + padding._bottom;

    // Store the new clipping region.
    skin->GetRender()->SetClippingRegion(bounds);
    skin->GetRender()->StartClipping();
}

void ListBox::Layout(Skin::Base* skin)
{
    // Call the base class function.
    ScrollControl::Layout(skin);

    // Update the table.
    const Gwen::Rectangle& inner_bounds = _inner_panel->GetInnerBounds();
    _table->SetPosition(inner_bounds._x, inner_bounds._y);
    _table->SetWidth(inner_bounds._width);
    _table->SizeToChildren(false, true);

    // Update the scroll bars.
    unsigned row_height = _table->GetDefaultRowHeight();
    _scroll_bar->SetNudgeAmount(static_cast<float>(row_height));
}

void ListBox::OnRowSelected(Base* control)
{
    bool clear = !Gwen::Input::IsShiftDown();
    if (!GetAllowMultiSelect())
    {
        clear = true;
    }

    SetSelectedRow(control, clear);
}

void ListBox::OnRowMouseDoubleClickLeft(Base* control)
{
    bool clear = !Gwen::Input::IsShiftDown();
    if (!GetAllowMultiSelect())
    {
        clear = true;
    }

    SetSelectedRow(control, clear);
    _on_row_mouse_double_click_left.Call(this);
}

bool ListBox::OnKeyDown(bool is_down)
{
    if (is_down)
    {
        std::list<Base*>& children = _table->GetChildren();
        const auto begin = children.begin();
        const auto end = children.end();

        Controls::Base* selected_row = GetSelectedRow();
        if (selected_row == nullptr && !children.empty())
        {
            // Select first element.
            selected_row = children.front();
        }

        auto result = std::find(begin, end, selected_row);
        if (result != end)
        {
            auto next = result;
            ++next;

            if (next != end)
            {
                result = next;
            }

            ListBoxRow* row = dynamic_cast<ListBoxRow*>(*result);
            if (row)
            {
                row->DoSelect();

                Controls::VerticalScrollBar* scroll_bar = dynamic_cast<Controls::VerticalScrollBar*>(_scroll_bar);
                if (scroll_bar)
                {
                    scroll_bar->NudgeDown(this);
                }

                Redraw();
            }
        }
    }

    return true;
}

bool ListBox::OnKeyUp(bool is_down)
{
    if (is_down)
    {
        std::list<Base*>& children = _table->GetChildren();
        const auto begin = children.begin();
        const auto end = children.end();

        Controls::Base* selected_row = GetSelectedRow();
        if (selected_row == nullptr && !children.empty())
        {
            selected_row = children.front();
        }

        auto result = std::find(begin, end, selected_row);
        if (result != end)
        {
            if (result != begin)
            {
                --result;
            }

            ListBoxRow* row = dynamic_cast<ListBoxRow*>(*result);
            if (row)
            {
                row->DoSelect();

                Controls::VerticalScrollBar* scroll_bar = dynamic_cast<Controls::VerticalScrollBar*>(_scroll_bar);
                if (scroll_bar)
                {
                    scroll_bar->NudgeUp(this);
                }

                Redraw();
            }
        }
    }

    return true;
}

}; // namespace Controls

}; // namespace Gwen
