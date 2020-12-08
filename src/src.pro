defineTest(minQtVersion) {
    maj = $$1
    min = $$2
    patch = $$3
    isEqual(QT_MAJOR_VERSION, $$maj) {
        isEqual(QT_MINOR_VERSION, $$min) {
            isEqual(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
            greaterThan(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
        }
        greaterThan(QT_MINOR_VERSION, $$min) {
            return(true)
        }
    }
    greaterThan(QT_MAJOR_VERSION, $$maj) {
        return(true)
    }
    return(false)
}

!minQtVersion(5, 8, 0) {
    message("Cannot build this demo with Qt version $${QT_VERSION}.")
    error("Use at least Qt 5.8.0.")
}

#######################################################################################################################

TEMPLATE = app
CONFIG -= debug_and_release debug_and_release_target

TOPDIR = $$clean_path($$PWD/..)

#######################################################################################################################
# Platform-specific setup

macx {
    TARGET = "Bitrix Forum Reader"

    # NOTE: Qt Creator unable to run application with different bundle and executable names
#    TARGET = "bitrix-forum-reader"
#    QMAKE_APPLICATION_BUNDLE_NAME = "Bitrix Forum Reader"
} else:ios {
    # FIXME: build failed on iOS if bundle name contains whitespaces
    #
    # PhaseScriptExecution Project\ Copy __BUILD__/client/debug/ios-arm64-gcc/Bitrix\ Forum\ Reader.build/Debug-iphonesimulator/Bitrix\ Forum\ Reader.build/Script-9C316F444A62BF296E3E1F25.sh
    #    cd /Users/eraxillan/Projects/bitrix-forum-reader
    #    /bin/sh -c \"/Users/eraxillan/Projects/bitrix-forum-reader/__BUILD__/client/debug/ios-arm64-gcc/Bitrix\ Forum\ Reader.build/Debug-iphonesimulator/Bitrix\ Forum\ Reader.build/Script-9C316F444A62BF296E3E1F25.sh\"
    # cp: /Users/eraxillan/Projects/bitrix-forum-reader/Debug-iphonesimulator/Bitrix: No such file or directory
    # cp: Forum: No such file or directory
    # cp: Reader.app: No such file or directory
    TARGET = "bitrix-forum-reader"
} else {
    TARGET = "bitrix-forum-reader"
}

darwin: QMAKE_RPATHDIR += @loader_path/../Frameworks

android {
    QT += svg widgets

    DISTFILES += \
        android/AndroidManifest.xml \
        android/res/values/libs.xml \
        android/build.gradle \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradle/wrapper/gradle-wrapper.properties

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
} else:macos {
    QMAKE_INFO_PLIST = Info.plist
} else:ios|tvos {
    QMAKE_INFO_PLIST = Info-ios.plist
}

android {
    DEFINES += QT_EXTRA_FILE_SELECTOR=\\\"android\\\"
} else:ios|tvos {
    DEFINES += QT_EXTRA_FILE_SELECTOR=\\\"ios\\\"
}

#######################################################################################################################

# Build mode (release by default)
#buildmode = release
#CONFIG(debug, debug|release):buildmode = debug

#APP_PLATFORM = $$first($$list($$QMAKE_PLATFORM))
#APP_ARCH = $$first($$list($$QT_ARCH))
#APP_COMPILER = $$first($$list($$QMAKE_COMPILER))
#APP_BUILD_DIR = $$TOPDIR/__BUILD__/client/$${buildmode}/$${APP_PLATFORM}-$${APP_ARCH}-$${APP_COMPILER}

#GUMBO_BUILD_DIR = $$TOPDIR/__BUILD__/gumbo/$${buildmode}/$${APP_PLATFORM}-$${APP_ARCH}-$${APP_COMPILER}

# Output directories setup
#DESTDIR     = $${APP_BUILD_DIR}
#UI_DIR      = $${APP_BUILD_DIR}
#OBJECTS_DIR = $${APP_BUILD_DIR}
#MOC_DIR     = $${APP_BUILD_DIR}

#######################################################################################################################

QT += qml quick quickcontrols2
QT += multimedia concurrent
# USE_QT_NAM: QT += network
# QT += gui widgets

FLUID_OUT_DIR = $$clean_path($$OUT_PWD/../fluid/qml)
GUMBO_PARSER_OUT_DIR = $$clean_path($$OUT_PWD/../gumbo-parser)

android {
    QT += androidextras

    # Bundle Fluid QML plugins with the application
    ANDROID_EXTRA_PLUGINS = $$FLUID_OUT_DIR

    # Android package sources
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}

CONFIG += c++17
INCLUDEPATH += "."
INCLUDEPATH += $$TOPDIR

# DEFINES += USE_QT_NAM
# DEFINES += QT_GUMBO_METADATA
# DEFINES += BFR_DRAW_FRAME_ON_COMPONENT_FOR_DEBUG
# DEFINES += BFR_PRINT_DEBUG_OUTPUT
# DEFINES += BFR_DUMP_GENERATED_QML_IN_FILES
# DEFINES += "BFR_QML_OUTPUT_DIR=\"\\\"__temp_qml\\\"\""
# DEFINES += BFR_SERIALIZATION_ENABLED

DEFINES += BFR_SHOW_SPOILER
DEFINES += BFR_SHOW_QUOTE
DEFINES += BFR_SHOW_IMAGE
DEFINES += BFR_SHOW_LINEBREAK
DEFINES += BFR_SHOW_PLAINTEXT
DEFINES += BFR_SHOW_RICHTEXT
DEFINES += BFR_SHOW_VIDEO
DEFINES += BFR_SHOW_HYPERLINK

#######################################################################################################################

SOURCES += \
    common/filedownloader.cpp               \
    common/forumthreadurl.cpp               \
    website_backend/gumboparserimpl.cpp     \
    website_backend/qtgumbodocument.cpp     \
    website_backend/qtgumbonode.cpp         \
    website_backend/websiteinterface.cpp    \
    website_backend/websiteinterface_qt.cpp \
    qml_frontend/forumreader.cpp            \
    qml_frontend/task.cpp                   \
    qml_frontend/main.cpp                   \
    parser_frontend/forumthreadpool.cpp     \
    tests/gumboparserimpl_tests.cpp

HEADERS += \
    common/filedownloader.h                 \
    common/forumthreadurl.h                 \
    common/logger.h                         \
    common/resultcode.h                     \
    website_backend/html_tag.h              \
    website_backend/qtgumbodocument.h       \
    website_backend/qtgumbonode.h           \
    website_backend/websiteinterface_fwd.h  \
    website_backend/websiteinterface.h      \
    website_backend/websiteinterface_qt.h   \
    website_backend/gumboparserimpl.h       \
    parser_frontend/forumthreadpool.h       \
    qml_frontend/forumreader.h              \
    qml_frontend/task.h

RESOURCES += qml_frontend/qml.qrc

#######################################################################################################################

# Static libraries for different architectures live in different directories
windows {
    contains(QT_ARCH, i386) {
        ARCH = x32
    } else {
        ARCH = x64
    }
} else:macx {
    # NOTE: modern macOS are 64-bit
    ARCH = x64
} else:linux:!android {
    ARCH = x64
} else:android {
    equals(QT_ARCH, "arm64-v8a") {
        ARCH = arm64-v8a
    } else:equals(QT_ARCH, "armeabi-v7a") {
        ARCH = armeabi-v7a
    } else:contains(QT_ARCH, "i386") {
        ARCH = x86
    } else {
        error("Unsupported Android architecture $$QT_ARCH!")
    }
} else:ios {
    ARCH = arm64
} else {
    error("Unsupported OS")
}

# ... and have different suffixes for debug and release modes
CONFIG(debug, debug|release) {
    SUFFIX = d
}

# FIXME: check whether Windows version can be build without external OpenSSL

# Link with:
#   1) OpenSSL library
#   2) cURL library (compiled with OpenSSL support)
#   3) Gumbo HTML5 parser library

INCLUDEPATH += $$TOPDIR/gumbo-parser/src
INCLUDEPATH += $$TOPDIR/spdlog/include

windows {
    # Windows (Desktop, X86_64, static libraries)
    DEFINES += CURL_STATICLIB
    INCLUDEPATH += $$TOPDIR/libs/curl/win32/include
    
    QMAKE_LFLAGS_DEBUG += /ignore:4099
    LIBS += -ladvapi32 -luser32 -lgdi32 -lws2_32 -lwsock32 -lWldap32

    # OpenSSL
    LIBS += -L$$TOPDIR/libs/openssl/win32/VC14/$$ARCH/$$buildmode
    # cURL
    LIBS += -L$$TOPDIR/libs/curl/win32/VC14/$$ARCH/$$buildmode
    # Gumbo
    LIBS += -L$$TOPDIR/libs/gumbo-parser/win32/VC14/$$ARCH/$$buildmode

    LIBS += -llibeay32 -lssleay32
    LIBS += -llibcurl$$SUFFIX
    LIBS += -lgumbo-parser
} else:macx {
    # cURL
    INCLUDEPATH += $$TOPDIR/libs/curl/macos/include
    LIBS += -L$$PWD/libs/curl/macos/lib
    LIBS += -lcurl

    # Gumbo
    LIBS += -L$$GUMBO_BUILD_DIR
    LIBS += -lgumbo-parser
} else:linux:!android {
    # Linux (Desktop, X86_64, shared libraries)

    # cURL
    # Just use system library installed:
    # Ubuntu: sudo apt-get install libcurl4-openssl-dev
    # Fedora: sudo dnf install libcurl-devel
    INCLUDEPATH += "/usr/include/x86_64-linux-gnu"
    LIBS += -L"/usr/lib/x86_64-linux-gnu"
    LIBS += -l"curl"

    # Gumbo
    LIBS += -L$$GUMBO_BUILD_DIR
    LIBS += -lgumbo-parser
} else:android {
    # Android (Mobile, X86/ARMv7/ARM64, static and shared libraries)

    # cURL: static
    INCLUDEPATH += $$TOPDIR/curl/prebuilt-with-ssl/android/include
    LIBS += -L$$TOPDIR/curl/prebuilt-with-ssl/android/$$ARCH
    LIBS += -lcurl

    # Gumbo: shared
    LIBS += -L$$GUMBO_PARSER_OUT_DIR
    LIBS += -lgumbo-parser_$$QT_ARCH
} else:ios {
    # iOS (Mobile, Universal binary, static libraries)

    # cURL
    INCLUDEPATH += $$TOPDIR/curl/prebuilt-with-ssl/iOS/include
    LIBS += -L$$TOPDIR/curl/prebuilt-with-ssl/iOS
    LIBS += -lcurl

    # Gumbo
    LIBS += -L$$GUMBO_BUILD_DIR
    LIBS += -lgumbo-parser
} else {
    error("Unsupported OS")
}

#######################################################################################################################

#CONFIG += fluid_resource_icons

QML_FILES += \
    $$TOPDIR/src/qml_frontend/qml/+android/main.qml \
    $$TOPDIR/src/qml_frontend/qml/+android/NavigationPanel.qml \
    $$TOPDIR/src/qml_frontend/qml/+android/PostList.qml \
    $$TOPDIR/src/qml_frontend/qml/+ios/main.qml \
    $$TOPDIR/src/qml_frontend/qml/main.qml \
    $$TOPDIR/src/qml_frontend/qml/PostAnimatedImage.qml \
    $$TOPDIR/src/qml_frontend/qml/PostHyperlink.qml \
    $$TOPDIR/src/qml_frontend/qml/PostImage.qml \
    $$TOPDIR/src/qml_frontend/qml/PostLineBreak.qml \
    $$TOPDIR/src/qml_frontend/qml/PostList.qml \
    $$TOPDIR/src/qml_frontend/qml/PostPlainText.qml \
    $$TOPDIR/src/qml_frontend/qml/Post.qml \
    $$TOPDIR/src/qml_frontend/qml/PostQuote.qml \
    $$TOPDIR/src/qml_frontend/qml/PostRichText.qml \
    $$TOPDIR/src/qml_frontend/qml/PostSpoiler.qml \
    $$TOPDIR/src/qml_frontend/qml/PostVideo.qml \
    $$TOPDIR/src/qml_frontend/qml/UserListDialog.qml \
    $$TOPDIR/src/qml_frontend/qml/User.qml

# Copy all files to the build directory so that QtCreator will recognize
# the QML module and the demo will run without installation
#isEmpty(DESTDIR): DESTDIR = $$OUT_PWD
#qmlfiles2build.files = $$QML_FILES
#qmlfiles2build.path = $$DESTDIR
#COPIES += qmlfiles2build

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH += $$FLUID_OUT_DIR
#QML_IMPORT_PATH += $$DESTDIR

#QML_IMPORT_PATH += $$TOPDIR/fluid/qml
#QML_IMPORT_PATH += $$OUT_PWD/qml_frontend/qml
#QML_IMPORT_PATH += $$OUT_PWD/qml_frontend/qml/+android
#QML_IMPORT_PATH += $$OUT_PWD/qml_frontend/qml/+ios

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment
include(deployment.pri)

ANDROID_ABIS = armeabi-v7a arm64-v8a
