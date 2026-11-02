"""Shared layout parameters/helpers for the MainMenu golden-hour cathedral vignette."""
import math
import unreal

# ---- World layout (cm) ----
G = 2500.0            # cathedral ground (ridge top) Z
ZB = -400.0           # foundation sink below G so bases bury into the ridge
CAM_LOC = unreal.Vector(0.0, 0.0, 2200.0)
CATH_YAW = 58.0       # local +X = nave axis (facade at local x=0 faces local -X)
P0 = (34000.0, -3000.0)   # facade centre (XY)
BAYS = 22
BAY = 675.0
NAVE_X0 = 480.0
NAVE_LEN = NAVE_X0 + BAYS * BAY
NAVE_HALF = 675.0
AISLE_Y = 1575.0
WALL_H = 625.0
WALL_ROWS = 4
TOWER_Y = 1250.0
TOWER_BP_BOTTOM = -2836.0
TOWER_BP_TOP = 5556.0

FOV_H = 40.0
ASPECT = 16.0 / 9.0
TAN_H = math.tan(math.radians(FOV_H / 2.0))
TAN_V = TAN_H / ASPECT

_ex = (math.cos(math.radians(CATH_YAW)), math.sin(math.radians(CATH_YAW)))
_ey = (-_ex[1], _ex[0])


def L(x, y, z):
    """Cathedral-local -> world Vector."""
    return unreal.Vector(P0[0] + x * _ex[0] + y * _ey[0], P0[1] + x * _ex[1] + y * _ey[1], G + z)


def cam_basis(yaw, pitch):
    cy, sy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    cp, sp = math.cos(math.radians(pitch)), math.sin(math.radians(pitch))
    F = (cp * cy, cp * sy, sp)
    R = (-sy, cy, 0.0)
    U = (-sp * cy, -sp * sy, cp)
    return F, R, U


def project(p, yaw, pitch, cam=CAM_LOC):
    F, R, U = cam_basis(yaw, pitch)
    d = (p.x - cam.x, p.y - cam.y, p.z - cam.z)
    x = d[0] * F[0] + d[1] * F[1] + d[2] * F[2]
    r = d[0] * R[0] + d[1] * R[1] + d[2] * R[2]
    u_ = d[0] * U[0] + d[1] * U[1] + d[2] * U[2]
    return r / (x * TAN_H), u_ / (x * TAN_V), x


def unproject(u, v, depth, yaw, pitch, cam=CAM_LOC):
    """World point seen at screen (u,v) in [-1,1] at forward depth."""
    F, R, U = cam_basis(yaw, pitch)
    k = (u * TAN_H, v * TAN_V)
    return unreal.Vector(cam.x + depth * (F[0] + k[0] * R[0] + k[1] * U[0]),
                         cam.y + depth * (F[1] + k[0] * R[1] + k[1] * U[1]),
                         cam.z + depth * (F[2] + k[0] * R[2] + k[1] * U[2]))


def key_points():
    pts = []
    for s in (-1, 1):
        for dx in (-476, 476):
            for dy in (-476, 476):
                pts.append(L(dx, s * TOWER_Y + dy, ZB))
        pts.append(L(0, s * TOWER_Y, ZB + (TOWER_BP_TOP - TOWER_BP_BOTTOM)))
        pts.append(L(NAVE_LEN + 400, s * AISLE_Y, ZB))
        pts.append(L(NAVE_X0, s * AISLE_Y, ZB))
    return pts


def solve_camera(u_left=-0.30, v_tip=0.62):
    yaw, pitch = 0.0, 7.0
    tips = [L(0, s * TOWER_Y, ZB + (TOWER_BP_TOP - TOWER_BP_BOTTOM)) for s in (-1, 1)]
    for _ in range(30):
        us = [project(p, yaw, pitch)[0] for p in key_points()]
        umin = min(us)
        yaw += math.degrees(math.atan(umin * TAN_H) - math.atan(u_left * TAN_H))
        vmax = max(project(p, yaw, pitch)[1] for p in tips)
        pitch += math.degrees(math.atan(vmax * TAN_V) - math.atan(v_tip * TAN_V))
    return yaw, pitch
