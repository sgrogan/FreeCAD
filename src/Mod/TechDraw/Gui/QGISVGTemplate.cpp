/***************************************************************************
 *   Copyright (c) 2012-2014 Luke Parry <l.parry@warwick.ac.uk>            *
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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QFile>
#include <QFont>
#include <QPen>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>
#include <strstream>
#include <string>
#include <boost/regex.hpp>
#endif // #ifndef _PreComp_

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Gui/Command.h>

#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>

#include "../App/GraphicsItems/ZVALUE.h"

#include "QGISVGTemplate.h"

using namespace TechDrawGui;

QGISVGTemplate::~QGISVGTemplate()
{
    clearContents();
}


void QGISVGTemplate::clearContents()
{
    for (std::vector<TemplateTextField *>::iterator it = textFields.begin();
            it != textFields.end(); ++it) {
        delete *it;
    }
    textFields.clear();
}


void QGISVGTemplate::draw()
{
    // Ensures that pageTemplate is valid and refers to a renderable SVG file
    if (!renderSvg())
        return;

    auto tmplte(static_cast<TechDraw::DrawSVGTemplate *>(pageTemplate));

    clearContents();

    // tmplte->PageResult is the file name of the template
    Base::FileInfo fi( tmplte->PageResult.getValue() );

    // make a temp file for FileIncluded Property
    std::string tempName = tmplte->PageResult.getExchangeTempFile();
    std::ostringstream ofile;
    std::string tempendl = "--endOfLine--";
    std::string line;

    std::ifstream ifile (fi.filePath().c_str());
    while (!ifile.eof())
    {
        std::getline(ifile,line);
        // check if the marker in the template is found
        if(line.find("<!-- DrawingContent -->") == std::string::npos) {
            // if not -  write through
            ofile << line << tempendl;
        }
    }

    std::string outfragment(ofile.str());

    // Find text tags with freecad:editable attribute and their matching tspans
    // keep tagRegex in sync with App/DrawSVGTemplate.cpp
    boost::regex tagRegex("<text([^>]*freecad:editable=[^>]*)>[^<]*<tspan[^>]*>([^<]*)</tspan>");

    // Smaller regexes for parsing matches to tagRegex
    boost::regex editableNameRegex("freecad:editable=\"(.*?)\"");
    boost::regex xRegex("x=\"([\\d.-]+)\"");
    boost::regex yRegex("y=\"([\\d.-]+)\"");
    //Note: some templates have fancy Transform clauses and don't use absolute x,y to position editableFields.
    //      editableFields will be in the wrong place in this case.

    std::string::const_iterator begin, end;
    begin = outfragment.begin();
    end = outfragment.end();
    boost::match_results<std::string::const_iterator> tagMatch, nameMatch, xMatch, yMatch;

    //TODO: Find location of special fields (first/third angle) and make graphics items for them

    // and update the sketch
    while (boost::regex_search(begin, end, tagMatch, tagRegex)) {
        if ( boost::regex_search(tagMatch[1].first, tagMatch[1].second, nameMatch, editableNameRegex) &&
             boost::regex_search(tagMatch[1].first, tagMatch[1].second, xMatch, xRegex) &&
             boost::regex_search(tagMatch[1].first, tagMatch[1].second, yMatch, yRegex) ) {

            //TODO: this should probably be configurable without a code change
            auto editClickBoxSize( 1.5 );
            auto editClickBoxColor( Qt::green );

            auto width( editClickBoxSize );
            auto height( editClickBoxSize );

            auto x( std::stod(xMatch[1].str()) ),
                 y( std::stod(yMatch[1].str()) );
            auto *item( new TemplateTextField(this, tmplte, nameMatch[1].str()) );
            auto pad( 1.0 );
            item->setRect(x - pad, -tmplte->getHeight() + y - height - pad,
                          width + 2 * pad, height + 2 * pad);

            QPen myPen;
            QBrush myBrush(editClickBoxColor,Qt::SolidPattern);
            myPen.setStyle(Qt::SolidLine);
            myPen.setColor(editClickBoxColor);
            myPen.setWidth(0);  // 0 means "cosmetic pen" - always 1px
            item->setPen(myPen);
            item->setBrush(myBrush);

            item->setZValue(ZVALUE::SVGTEMPLATE);
            addToGroup(item);
            textFields.push_back(item);
        }

        begin = tagMatch[0].second;
    }
}

void QGISVGTemplate::updateView(bool update)
{
    draw();
}

