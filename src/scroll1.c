
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
