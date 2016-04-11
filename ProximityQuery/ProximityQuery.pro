#-------------------------------------------------
#
# Project created by QtCreator 2016-01-30T10:50:57
#
#-------------------------------------------------

QT       -= core gui

TARGET = ProximityQuery
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

CONFIG += link_pkgconfig

PKGCONFIG += glfw3 glew

INCLUDEPATH += ../include

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp \
    imgui/imgui.cpp \
    imgui/imguiRenderGL3.cpp \
    ObjLoader.cpp \
    Render.cpp \
    TriMesh.cpp \
	TrackBall.cpp

HEADERS += \
    imgui/imgui.h \
    imgui/imguiRenderGL3.h \
    imgui/stb_truetype.h \
    ObjLoader.hpp \
    Render.hpp \
    TriMesh.hpp \
	TrackBall.cpp

OTHER_FILES += \
    fsLine.glsl \
    fsMesh.glsl \
    vsLine.glsl \
    vsMesh.glsl
