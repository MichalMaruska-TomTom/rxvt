/* INTPROTO */
void
cleanup_cursor_and_outline(rxvt_t *r, screen_t *screen, rend_t cc1, rend_t cc2,
			   unsigned char morecur)
{
    /* Added: */
    rend_t	   *srp;
    struct rxvt_hidden *h = r->h;


    if (r->TermWin.focus) {
	srp = &(screen->rend[screen->cur.row + r->TermWin.saveLines]
		[screen->cur.col]);
	*srp ^= RS_RVid;
#ifndef NO_CURSORCOLOR
	*srp = (*srp & ~(RS_fgMask | RS_bgMask)) | cc1;
#endif
#ifdef MULTICHAR_SET
	if (morecur) {
	    srp += morecur;
	    *srp ^= RS_RVid;
# ifndef NO_CURSORCOLOR
	    *srp = (*srp & ~(RS_fgMask | RS_bgMask)) | cc2;
# endif
	}
#endif
    } else if (h->oldcursor.row >= 0) {
#ifndef NO_CURSORCOLOR
	unsigned long	gcmask;	/* Graphics Context mask */

	gcmask = 0;
	XGCValues      gcvalue;
	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_cursor)) {
	    gcvalue.foreground = r->PixColors[Color_cursor];
	    gcmask = GCForeground;
	    XChangeGC(r->Xdisplay, r->TermWin.gc, gcmask, &gcvalue);
	    gcvalue.foreground = r->PixColors[Color_fg];
	}
#endif
#if 1				/* mmc! */
	D_SCREEN((stderr, "%s XDrawRectangle: (%d, %d -- %d, %d)\n",
		  __FUNCTION__,
		  Col2Pixel(h->oldcursor.col + morecur),
		  Row2Pixel(h->oldcursor.row),
		  (unsigned int)(Width2Pixel(1 + (morecur ? 1 : 0))
				 - 1),
		  (unsigned int)(Height2Pixel(1)
				 - r->TermWin.lineSpace - 1)
		     ));
#endif
	XDrawRectangle(r->Xdisplay, drawBuffer, r->TermWin.gc,
		       Col2Pixel(h->oldcursor.col + morecur),
		       Row2Pixel(h->oldcursor.row),
		       (unsigned int)(Width2Pixel(1 + (morecur ? 1 : 0))
				      - 1),
		       (unsigned int)(Height2Pixel(1)
				      - r->TermWin.lineSpace - 1));
#ifndef NO_CURSORCOLOR
	if (gcmask)		/* restore normal colours */
	    XChangeGC(r->Xdisplay, r->TermWin.gc, gcmask, &gcvalue);
#endif
    }
}

/* INTPROTO */
void
clear_borders(rxvt_t *r, unsigned char clearfirst, unsigned char clearlast)
{

    /* mmc! Clear the first column:  The left-most border ? */
    if (clearfirst && r->TermWin.int_bwidth) {

	/*
	 * clear the whole screen height, note that width == 0 is treated
	 * specially by XClearArea
	 */
	D_SCREEN((stderr, "XClearArea FIRST %d x %d?",
		  (unsigned int)r->TermWin.int_bwidth,
		  (unsigned int)TermWin_TotalHeight()));

	XClearArea(r->Xdisplay,	 drawBuffer, 0, 0, /* r->TermWin.vt */
		   (unsigned int)r->TermWin.int_bwidth,
		   /* mmc: What is this?       Internal border width!*/
		   (unsigned int)TermWin_TotalHeight(), False);
    }

    if (clearlast && r->TermWin.int_bwidth) {

	D_SCREEN((stderr, "XClearArea LAST %d x %d @ %d?",
		  (unsigned int)r->TermWin.int_bwidth,
		  (unsigned int)TermWin_TotalHeight(),
		  r->TermWin.width + r->TermWin.int_bwidth
		     ));
	/*
	 * clear the whole screen height, note that width == 0 is treated
	 * specially by XClearArea
	 */
	XClearArea(r->Xdisplay, drawBuffer, /* r->TermWin.vt */
		   r->TermWin.width + r->TermWin.int_bwidth, 0,
		   (unsigned int)r->TermWin.int_bwidth,
		   (unsigned int)TermWin_TotalHeight(), False);
    }
}
