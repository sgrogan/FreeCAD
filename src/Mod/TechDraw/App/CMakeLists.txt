if(MSVC)
    add_definitions(-DFCAppTechDraw -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H -DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${QT_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${QT_QTCORE_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(TechDrawLIBS
    Measure
    Part
    FreeCADApp
)

if(QT_QTGUI_LIBRARY AND QT_QTSVG_LIBRARY)
    list(APPEND TechDrawLIBS ${QT_QTGUI_LIBRARY} ${QT_QTSVG_LIBRARY})

else(QT_QTGUI_LIBRARY AND QT_QTSVG_LIBRARY)
    # TODO: Try to get this to a point where we can compile a reduced-
    # functionality version of App that doesn't need QGraphics stuff.
    message(SEND_ERROR
            "QT_QTGUI_LIBRARY or QT_QTSVG_LIBRARY not found - can't use TechDraw.")

endif(QT_QTGUI_LIBRARY AND QT_QTSVG_LIBRARY)

generate_from_xml(DrawPagePy)
generate_from_xml(DrawViewPy)
generate_from_xml(DrawViewPartPy)
generate_from_xml(DrawViewSymbolPy)
generate_from_xml(DrawTemplatePy)
generate_from_xml(DrawParametricTemplatePy)
generate_from_xml(DrawSVGTemplatePy)
generate_from_xml(DrawViewSymbolPy)
generate_from_xml(DrawViewClipPy)
generate_from_xml(DrawViewDimensionPy)
generate_from_xml(DrawHatchPy)
generate_from_xml(DrawViewCollectionPy)
generate_from_xml(DrawProjGroupPy)
generate_from_xml(DrawProjGroupItemPy)

SET(DrawGI_MOC_HDRS
    GIPage.h
   ) # end of SET(DrawGI_MOC_HDRS

fc_wrap_cpp(TechDraw_MOC_SRCS ${DrawGI_MOC_HDRS})
SOURCE_GROUP("TechDraw Moc" FILES ${TechDraw_MOC_SRCS})

SET(DrawGI_SRCS
    GIBase.h
    GIBase.cpp
    GIPage.cpp
    ${DrawGI_MOC_HDRS}
   ) # end of SET(QGI_SRCS

SET(Draw_SRCS
    DrawPage.cpp
    DrawPage.h
    DrawView.cpp
    DrawView.h
    DrawViewPart.cpp
    DrawViewPart.h
    DrawViewAnnotation.cpp
    DrawViewAnnotation.h
    DrawViewSymbol.cpp
    DrawViewSymbol.h
    DrawViewClip.cpp
    DrawViewClip.h
    DrawProjGroup.cpp
    DrawProjGroup.h
    DrawProjGroupItem.cpp
    DrawProjGroupItem.h
    DrawParametricTemplate.cpp
    DrawParametricTemplate.h
    DrawSVGTemplate.cpp
    DrawSVGTemplate.h
    DrawTemplate.cpp
    DrawTemplate.h
    DrawViewCollection.cpp
    DrawViewCollection.h
    DrawViewDimension.cpp
    DrawViewDimension.h
    DrawViewSection.cpp
    DrawViewSection.h
    DrawHatch.cpp
    DrawHatch.h
    DrawViewDraft.cpp
    DrawViewDraft.h)

SET(TechDraw_SRCS
    AppTechDraw.cpp
    AppTechDrawPy.cpp
    DrawUtil.cpp
    DrawUtil.h
    PreCompiled.cpp
    PreCompiled.h
)

SET(Geometry_SRCS
    Geometry.cpp
    Geometry.h
    GeometryObject.cpp
    GeometryObject.h
)

SET(Python_SRCS
    DrawPagePy.xml
    DrawPagePyImp.cpp
    DrawViewPy.xml
    DrawViewPyImp.cpp
    DrawViewPartPy.xml
    DrawViewPartPyImp.cpp
    DrawViewSymbolPy.xml
    DrawViewSymbolPyImp.cpp
    DrawTemplatePy.xml
    DrawTemplatePyImp.cpp
    DrawParametricTemplatePy.xml
    DrawParametricTemplatePyImp.cpp
    DrawSVGTemplatePy.xml
    DrawSVGTemplatePyImp.cpp
    DrawViewClipPy.xml
    DrawViewClipPyImp.cpp
    DrawViewDimensionPy.xml
    DrawViewDimensionPyImp.cpp
    DrawHatchPy.xml
    DrawHatchPyImp.cpp
    DrawViewCollectionPy.xml
    DrawViewCollectionPyImp.cpp
    DrawProjGroupPy.xml
    DrawProjGroupPyImp.cpp
    DrawProjGroupItemPy.xml
    DrawProjGroupItemPyImp.cpp)

SOURCE_GROUP("Mod" FILES ${TechDraw_SRCS})
SOURCE_GROUP("Features" FILES ${Draw_SRCS})
SOURCE_GROUP("Geometry" FILES ${Geometry_SRCS})
SOURCE_GROUP("Python" FILES ${Python_SRCS})
SOURCE_GROUP("GraphicsItems" FILES ${DrawGI_SRCS})


if(MSVC)
    #add_definitions(-D_PreComp_)
    #GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" TechDrawCPP_SRCS ${TechDraw_SRCS} ${Draw_SRCS} )
    #ADD_MSVC_PRECOMPILED_HEADER(TechDraw PreCompiled.h PreCompiled.cpp TechDrawCPP_SRCS)
endif(MSVC)

add_library(TechDraw SHARED ${TechDraw_SRCS} ${Draw_SRCS} ${TechDrawAlgos_SRCS}
                            ${Geometry_SRCS} ${Python_SRCS} ${DrawGI_SRCS})
target_link_libraries(TechDraw ${TechDrawLIBS};${QT_QTXML_LIBRARY};${TechDraw})

SET(TechDrawScripts
    Init.py
)

fc_target_copy_resource(TechDraw
    ${CMAKE_SOURCE_DIR}/src/Mod/TechDraw
    ${CMAKE_BINARY_DIR}/Mod/TechDraw
    ${TechDrawScripts})

fc_target_copy_resource(TechDraw
    ${CMAKE_SOURCE_DIR}/src/Mod/TechDraw
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    ${TechDrawTemplates})

SET_BIN_DIR(TechDraw TechDraw /Mod/TechDraw)
SET_PYTHON_PREFIX_SUFFIX(TechDraw)

INSTALL(TARGETS TechDraw DESTINATION ${CMAKE_INSTALL_LIBDIR})
