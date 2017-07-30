
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
// Copyright (c) 2014-2017 Francois Beaune, The appleseedhq Organization
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

// Interface header.
#include "serialrenderercontroller.h"

// appleseed.renderer headers.
#include "renderer/kernel/rendering/itilecallback.h"

// appleseed.foundation headers.
#include "foundation/utility/otherwise.h"

namespace renderer
{

//
// SerialRendererController class implementation.
//

SerialRendererController::SerialRendererController(
    IRendererController*    controller,
    ITileCallback*          tile_callback)
  : m_controller(controller)
  , m_tile_callback(tile_callback)
{
}

void SerialRendererController::on_rendering_begin()
{
    m_controller->on_rendering_begin();
}

void SerialRendererController::on_rendering_success()
{
    // Execute any pending callback since the last time on_progress() was called.
    exec_callbacks();

    m_controller->on_rendering_success();
}

void SerialRendererController::on_rendering_abort()
{
    // Execute any pending callback since the last time on_progress() was called.
    exec_callbacks();

    m_controller->on_rendering_abort();
}

void SerialRendererController::on_frame_begin()
{
    m_controller->on_frame_begin();
}

void SerialRendererController::on_frame_end()
{
    m_controller->on_frame_end();
}

void SerialRendererController::on_progress()
{
    exec_callbacks();
    m_controller->on_progress();
}

IRendererController::Status SerialRendererController::get_status() const
{
    return m_controller->get_status();
}

void SerialRendererController::add_on_tile_begin_callback(
    const Frame*            frame,
    const size_t            tile_x,
    const size_t            tile_y)
{
    boost::mutex::scoped_lock lock(m_mutex);

    PendingTileCallback callback;
    callback.m_type = PendingTileCallback::OnTileBegin;
    callback.m_frame = frame;
    callback.m_tile_x = tile_x;
    callback.m_tile_y = tile_y;

    m_pending_callbacks.push_back(callback);
}

void SerialRendererController::add_on_tile_end_callback(
    const Frame*            frame,
    const size_t            tile_x,
    const size_t            tile_y)
{
    boost::mutex::scoped_lock lock(m_mutex);

    PendingTileCallback callback;
    callback.m_type = PendingTileCallback::OnTileEnd;
    callback.m_frame = frame;
    callback.m_tile_x = tile_x;
    callback.m_tile_y = tile_y;

    m_pending_callbacks.push_back(callback);
}

void SerialRendererController::add_on_progressive_frame_end_callback(const Frame* frame)
{
    boost::mutex::scoped_lock lock(m_mutex);

    PendingTileCallback callback;
    callback.m_type = PendingTileCallback::OnProgressiveFrameEnd;
    callback.m_frame = frame;
    callback.m_tile_x = 0;
    callback.m_tile_y = 0;

    m_pending_callbacks.push_back(callback);
}

void SerialRendererController::exec_callback(const PendingTileCallback& cb)
{
    switch (cb.m_type)
    {
      case PendingTileCallback::OnTileBegin:
        m_tile_callback->on_tile_begin(cb.m_frame, cb.m_tile_x, cb.m_tile_y);
        break;

      case PendingTileCallback::OnTileEnd:
        m_tile_callback->on_tile_end(cb.m_frame, cb.m_tile_x, cb.m_tile_y);
        break;

      case PendingTileCallback::OnProgressiveFrameEnd:
        m_tile_callback->on_progressive_frame_end(cb.m_frame);
        break;

      assert_otherwise;
    }
}

void SerialRendererController::exec_callbacks()
{
    boost::mutex::scoped_lock lock(m_mutex);

    while (!m_pending_callbacks.empty())
    {
        exec_callback(m_pending_callbacks.front());
        m_pending_callbacks.pop_front();
    }
}

}   // namespace renderer
