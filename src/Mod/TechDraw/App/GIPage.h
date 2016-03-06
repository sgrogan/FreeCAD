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

#ifndef GIVIEWPAGE_HEADER
#define GIVIEWPAGE_HEADER

#include <QGraphicsView>

#include "DrawPage.h"

namespace TechDraw {

class TechDrawExport GIPage : public QGraphicsView
{
    Q_OBJECT
public:
    GIPage(DrawPage *page, QWidget *parent = 0);
    ~GIPage() = default;

    /// Renders the page to SVG with filename.
    virtual void saveSvg(QString filename);

protected:
    DrawPage *m_page;    
};  // end class GIPage

};  // end namespace TechDraw

#endif // #ifndef GIVIEWPAGE_HEADER

