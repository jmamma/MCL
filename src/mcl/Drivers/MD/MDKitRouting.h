#pragma once

#include <stdint.h>

// Physical Machinedrum destinations use the Global wire encoding: A-F=0-5,
// MAIN=6. A kit may instead inherit the active Global with the 7-bit value 127.
constexpr uint8_t MD_KIT_ROUTE_GLOBAL = 127u;
constexpr bool mdKitRouteValid(uint8_t route) {
    return route <= 6u || route == MD_KIT_ROUTE_GLOBAL;
}
constexpr uint8_t mdKitEffectiveRoute(uint8_t route, uint8_t global) {
    return route == MD_KIT_ROUTE_GLOBAL ? global : route;
}
constexpr int mdKitRouteSelection(uint8_t route) {
    return route == MD_KIT_ROUTE_GLOBAL ? 0 : route == 6u ? 1 : route + 2;
}
constexpr uint8_t mdKitRouteFromSelection(int selection) {
    return selection == 0 ? MD_KIT_ROUTE_GLOBAL
         : selection == 1 ? 6u : static_cast<uint8_t>(selection - 2);
}
inline const char* mdKitRouteLabel(uint8_t route) {
    static const char* const labels[] = {"--", "MA", "A", "B", "C", "D", "E", "F"};
    return mdKitRouteValid(route) ? labels[mdKitRouteSelection(route)] : "--";
}
