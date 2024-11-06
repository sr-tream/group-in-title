cmake_minimum_required( VERSION 3.27 )

project( group-in-title
    DESCRIPTION "A hyprland plugin to add group id in title"
    VERSION 0.1
)

add_library( group-in-title SHARED main.cpp )

find_package( PkgConfig REQUIRED )
pkg_check_modules( deps REQUIRED IMPORTED_TARGET
    hyprland
    libdrm
    libinput
    libudev
    pangocairo
    pixman-1
    wayland-server
    xkbcommon
)
target_link_libraries( group-in-title PRIVATE rt PkgConfig::deps )

set_target_properties( group-in-title PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

install( TARGETS group-in-title )
