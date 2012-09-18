
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2012 Francois Beaune, Jupiter Jazz Limited
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
#include "assemblyinstanceitem.h"

// appleseed.studio headers.
#include "mainwindow/project/projectbuilder.h"

// appleseed.renderer headers.
#include "renderer/api/project.h"
#include "renderer/api/scene.h"

// appleseed.foundation headers.
#include "foundation/utility/uid.h"

using namespace foundation;
using namespace renderer;

namespace appleseed {
namespace studio {

AssemblyInstanceItem::AssemblyInstanceItem(
    AssemblyInstance*               assembly_instance,
    BaseGroup&                      parent,
    AssemblyInstanceCollectionItem* collection_item,
    ProjectBuilder&                 project_builder)
  : EntityItemBase<AssemblyInstance>(assembly_instance)
  , m_parent(parent)
  , m_collection_item(collection_item)
  , m_project_builder(project_builder)
{
    set_allow_edition(false);
}

void AssemblyInstanceItem::slot_delete()
{
    if (!allows_deletion())
        return;

    const UniqueID assembly_instance_uid = m_entity->get_uid();

    // Remove and delete the assembly instance.
    m_parent.assembly_instances().remove(assembly_instance_uid);
    
    // Mark the scene and the project as modified.
    m_project_builder.get_project().get_scene()->bump_version_id();
    m_project_builder.notify_project_modification();

    // Remove and delete the assembly instance item.
    m_collection_item->delete_item(assembly_instance_uid);

    // At this point 'this' no longer exists.
}

}   // namespace studio
}   // namespace appleseed
