#include <libgpu.h>
#include <libgs.h>
#include <libgte.h>

#include <dw/aabb.h>
#include <dw/entity.h>
#include <dw/garbage.h>
#include <dw/params.h>
#include <dw/types.h>

extern GsOT *ACTIVE_ORDERING_TABLE;

void setRotTransMatrix(MATRIX *m);
int32_t getTileTrigger(VECTOR *loc);
void renderDropShadow(Entity *entity);

#ifdef DW1_NATIVE
extern int32_t DRAWING_OFFSET_X;
extern int32_t DRAWING_OFFSET_Y;
void getModelTile(VECTOR *pos, int16_t *outTileX, int16_t *outTileY);

/* A separating screen edge proves the projected quad cannot cover a visible pixel.
 * Keep a four-pixel margin for the HD precise-vertex tolerance. Unlike corner-inclusion
 * tests this retains large quads that cross/enclose the screen with every vertex outside. */
static int32_t dw1_shadow_wholly_offscreen(const POLY_FT4 *p, uint32_t flag)
{
	int64_t left = -(int64_t)DRAWING_OFFSET_X - 4;
	int64_t right = 319 - (int64_t)DRAWING_OFFSET_X + 4;
	int64_t top = -(int64_t)DRAWING_OFFSET_Y - 4;
	int64_t bottom = 239 - (int64_t)DRAWING_OFFSET_Y + 4;

	/* MAC/IR or depth saturation can invalidate the precise-position bound. Screen
	 * XY saturation alone is monotone if the expanded viewport is inside its range. */
	if ((flag & 0x7F840000u) || left < -1024 || right > 1023 || top < -1024 || bottom > 1023) {
		return 0;
	}

	return (p->x0 < left && p->x1 < left && p->x2 < left && p->x3 < left) ||
	       (p->x0 > right && p->x1 > right && p->x2 > right && p->x3 > right) ||
	       (p->y0 < top && p->y1 < top && p->y2 < top && p->y3 < top) ||
	       (p->y0 > bottom && p->y1 > bottom && p->y2 > bottom && p->y3 > bottom);
}
#endif

GARBAGE(renderDropShadow, 19);

void renderDropShadow(Entity *entity)
{
	SVECTOR p0;
	SVECTOR p1;
	SVECTOR p2;
	SVECTOR p3;
	long depth;
	long flag;
	GsOT_TAG *ot;
	AABB box;
	SVECTOR c;
	POLY_FT4 *prim;
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t radiusX;
	int16_t radiusZ;
#ifdef DW1_NATIVE
	int16_t tileX;
	int16_t tileY;
	int32_t represented;

	getModelTile(&entity->posData->location, &tileX, &tileY);
	represented = dw1_collision_index_is_represented(tileX + tileY * 100);
	if (represented) {
#endif

	if (getTileTrigger(&entity->posData->location) == -1) {
		return;
	}
#ifdef DW1_NATIVE
	}
#endif
	radiusX = DIGIMON_DATA[entity->type].radius;
	radiusZ = DIGIMON_DATA[entity->type].radius;
	x = entity->posData->location.vx;
	y = entity->posData->location.vy;
	z = entity->posData->location.vz;
	p0.vx = x - radiusX;
	p0.vy = y;
	p0.vz = z - radiusZ;
	p1.vx = x + radiusX;
	p1.vy = y;
	p1.vz = z - radiusZ;
	p2.vx = x - radiusX;
	p2.vy = y;
	p2.vz = z + radiusZ;
	p3.vx = x + radiusX;
	p3.vy = y;
	p3.vz = z + radiusZ;
	prim = (POLY_FT4 *)GsGetWorkBase();
	SetPolyFT4(prim);
	SetSemiTrans(prim, 1);
	prim->tpage = getTPage(1, 2, 832, 256);
	setClut(prim, 0, 0x1E7);
	setUVWH(prim, 0x40, 0x80, 63, 63);
	setRotTransMatrix(&GsWSMATRIX);
	RotTransPers4(&p0, &p1, &p2, &p3, (long *)&prim->x0, (long *)&prim->x1, (long *)&prim->x2,
	              (long *)&prim->x3, &depth, &flag);
#ifdef DW1_NATIVE
	if (!represented) {
		/* Battle-end retreat can outlive the screen flag. Do not read unsupported
		 * neighbouring PS1 globals just to draw a wholly invisible shadow. Potentially
		 * visible quads still use the original strict collision guard. */
		if (dw1_shadow_wholly_offscreen(prim, flag)) {
			return;
		}
		getTileTrigger(&entity->posData->location);
	}
#endif
	prim->r0 = prim->g0 = prim->b0 = 0x30;
	ot = ACTIVE_ORDERING_TABLE->org;
	AddPrim(ot + 0xFFD, prim);
	prim++;
	GsSetWorkBase((PACKET *)prim);
	c.vx = entity->posData->location.vx;
	c.vy = -(DIGIMON_DATA[entity->type].height >> 1);
	c.vz = entity->posData->location.vz;
	box.center = &c;
	box.extent.vx = DIGIMON_DATA[entity->type].radius;
	box.extent.vy = DIGIMON_DATA[entity->type].height >> 1;
	box.extent.vz = DIGIMON_DATA[entity->type].radius;
	renderAABB(&box);
}
