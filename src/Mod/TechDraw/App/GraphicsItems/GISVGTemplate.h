/***************************************************************************
 *   Copyright (c) 2016                    Ian Rees <ian.rees@gmail.com>   *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef GISVGTEMPLATE_HEADER
#define GISVGTEMPLATE_HEADER

#include <memory>

#include "GITemplate.h"

QT_BEGIN_NAMESPACE
class QSvgRenderer;
class QGraphicsSvgItem;
QT_END_NAMESPACE

namespace TechDraw {

/// Uses a SVG image as the template/background and allows modification of text
class TechDrawExport GISVGTemplate : virtual public GITemplate
{
public:
    GISVGTemplate();
    virtual ~GISVGTemplate() = default;

    enum {Type = QGraphicsItem::UserType + 153};
    int type() const { return Type; }

    virtual void draw();

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    /// helper for draw(), but has return value used by QGISVGTemplate
    bool renderSvg();
    
    // TODO: Smart pointer?
    QGraphicsSvgItem *m_svgItem;
    
    /// Does the heavy lifting
    std::unique_ptr<QSvgRenderer> m_svgRender;

};  // end class GISVGTemplate

};  // end namespace TechDraw

#endif // #ifndef GISVGTEMPLATE_HEADER

