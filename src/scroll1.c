/* this file contains functions extracted from the original `rxvt_scr_refresh' */

#define DRAW_STRING(Func, x, y, str, len)				\
    Func(r->Xdisplay, drawBuffer, r->TermWin.gc, (x), (y), (str), (len))

#if defined (NO_BRIGHTCOLOR) || defined (VERYBOLD)
# define MONO_BOLD(x)		((x) & (RS_Bold|RS_Blink))
# define MONO_BOLD_FG(x, fg)	MONO_BOLD(x)
#else
# define MONO_BOLD(x)						\
    (((x) & (RS_Bold | RS_fgMask)) == (RS_Bold | Color_fg))
# define MONO_BOLD_FG(x, fg)	(((x) & RS_Bold) && (fg) == Color_fg)
#endif

#define FONT_WIDTH(X, Y)						\
    (X)->per_char[(Y) - (X)->min_char_or_byte2].width
#define FONT_RBEAR(X, Y)						\
    (X)->per_char[(Y) - (X)->min_char_or_byte2].rbearing
#define FONT_LBEAR(X, Y)						\
    (X)->per_char[(Y) - (X)->min_char_or_byte2].lbearing
#define IS_FONT_CHAR(X, Y)						\
    ((Y) >= (X)->min_char_or_byte2 && (Y) <= (X)->max_char_or_byte2)



#ifndef NO_PIXEL_DROPPING_AVOIDANCE
  /*
   * E1: pixel dropping avoidance.  Do this before the main refresh on the line.
   *     Require a refresh where pixels may have been dropped into our
   *     area by a neighbour character which has now changed
   *     TODO: this could be integrated into E2 but might be too messy
   * The result is .... modifying drawn_text so that it results changed, surely,
   * in the following step.
   */
/* inline */ void
mark_damaged_chars_from_left(rxvt_t *r,
			     text_t *stp,rend_t *srp,
			     text_t *dtp, rend_t *drp,
			     unsigned char *clearfirst, unsigned char *clearlast)
{
    int	  j;
    unsigned char	  clear_next = 0;
    const XFontStruct *wf;	/* which font are we in */
    int16_t	  col;

# ifdef NO_BOLDFONT
    wf = r->TermWin.font;
# endif

    for (col = 0; col < r->TermWin.ncol; col++) {
	unsigned char   is_font_char, is_same_char;
	text_t	    t;

	t = dtp[col];
	is_same_char = (t == stp[col] && drp[col] == srp[col]);

	if (!clear_next
	    && (is_same_char
		|| t == 0	/* screen cleared elsewhere */
		|| t == ' ')
	    ) {
	  if (!(drp[col] & RS_needs_redraw)) { /* just exposed */
	    /* fprintf(stderr,"%s%d/%d RS_needs_redraw%s\n", color_red, row, col,
	       color_reset);*/
	    continue;
	  }
	}
	if (clear_next) {	/* previous char caused change here */
	  clear_next = 0;
	  /* mmc: we simulate, as if .... */
	  /* Drawn text is Zeroed. todo: set `RS_needs_redraw' instead! */
	  dtp[col] = 0;
	  if (is_same_char)	/* don't cascade into next char */
	    continue;
	}
	j = MONO_BOLD(drp[col]) ? 1 : 0;
# ifndef NO_BOLDFONT
	wf = (j && r->TermWin.boldFont) ? r->TermWin.boldFont
	    : r->TermWin.font;
# endif
	/*
	 * TODO: consider if anything special needs to happen with:
	 * #if defined(MULTICHAR_SET) && ! defined(NO_BOLDOVERSTRIKE_MULTI)
	 */
	is_font_char = (wf->per_char && IS_FONT_CHAR(wf, t)) ? 1 : 0;
	if (!is_font_char || FONT_LBEAR(wf, t) < 0) {
	    if (col == 0)
		*clearfirst = 1;
	    else
		dtp[col - 1] = 0;
	}
	if (!is_font_char
	    || (FONT_WIDTH(wf, t) < (FONT_RBEAR(wf, t) + j))) {
	    if (col == r->TermWin.ncol - 1)
		*clearlast = 1;
	    else
		clear_next = 1;
	}
    }
}
#endif /* NO_PIXEL_DROPPING_AVOIDANCE */

/* return TRUE if no redraw needed.
 * can increase *ret_col */
inline int
cell_stays_unchanged(rend_t rend, int16_t* ret_col,
		     text_t* stp, text_t* dtp, rend_t* drp)
{
    int col = *ret_col;

    /* fixme: here bug?	  srp is 0!*/
    /* screen rendition (target rendtion) */
    if (stp[col] == dtp[col]	/* same characters */
	&& (rend == drp[col]	/* Either rendition the same or	 */
	    || (stp[col] == ' '	/* (both) space w/ no background change*/
		&& GET_BGATTR(rend) == GET_BGATTR(drp[col])))){
	/* but if exposed: */
	if (drp[col] & RS_needs_redraw)
	    return FALSE;

	if (!IS_MULTI1(rend))
	    return TRUE;   /* This displayed rectangle needs no change!	 Hurra! */
#ifdef MULTICHAR_SET
	else {	/* first byte is Kanji so compare second bytes */
	    if (stp[col + 1] == dtp[col + 1]){
		/* assume no corrupt characters on the screen */
		(*ret_col)++;
		return TRUE;
	    }
	}
#endif
    }
    return FALSE;
}

typedef int(*draw_function) ();

// determines the function to draw the glyphs.
inline void
find_longest_string_to_draw(rxvt_t *r,
			    rend_t	  rend,
			    unsigned char fprop,
			    int16_t	  *ret_len,
			    int16_t	  *wlen,
			    char	  *buffer,
			    int16_t	  *ret_col,
			    unsigned char *fontdiff,
			    text_t* stp,
			    rend_t* srp,
			    text_t* dtp,
			    rend_t* drp,
			    draw_function *draw_string,
			    draw_function *draw_image_string,
			    unsigned char *wbyte,
			    unsigned char must_clear)
{
    int col = *ret_col;
    int len = *ret_len;

#ifdef MULTICHAR_SET
    if (IS_MULTI1(rend)
	&& col < r->TermWin.ncol - 1 && IS_MULTI2(srp[col + 1])){
	if (!*wbyte && r->TermWin.mfont){
	    *wbyte = 1;
	    XSetFont(r->Xdisplay, r->TermWin.gc,
		     r->TermWin.mfont->fid);
	    *fontdiff = (r->TermWin.propfont & PROPFONT_MULTI);

	    *draw_string = XDrawString16;
	    *draw_image_string = XDrawImageString16;
	}
	if (r->TermWin.mfont == NULL){
	    buffer[0] = buffer[1] = ' ';
	    len = 2;
	    col++;
	} else {
	    /* double stepping - we're in multibyte font mode */
	    for (; ++col < r->TermWin.ncol;) {
		/* XXX: could check sanity on 2nd byte */
		dtp[col] = stp[col];
		drp[col] = srp[col];
		buffer[len++] = stp[col];
		col++;
		if (fprop)	/* proportional multibyte font mode */
		    break;
		if ((col == r->TermWin.ncol) || (srp[col] != rend))
		    break;
		if ((stp[col] == dtp[col])
		    && (srp[col] == drp[col])
		    && (stp[col + 1] == dtp[col + 1]))
		    break;
		if (len == h->currmaxcol)
		    break;
		dtp[col] = stp[col];
		drp[col] = srp[col];
		buffer[len++] = stp[col];
	    }
	    col--;
	}
	if (buffer[0] & 0x80)
	  (h->multichar_decode)((unsigned char*) buffer, len); /* fixme! */
	*wlen = len / 2;
    } else {
	if (rend & RS_multi1) {
	    /* corrupt character - you're outta there */
	    rend &= ~RS_multiMask;
	    drp[col] = rend;	/* TODO check: may also want */
	    dtp[col] = ' ';	/* to poke into stp/srp	     */
	    buffer[0] = ' ';
	}
	if (*wbyte) {
	    *wbyte = 0;
	    XSetFont(r->Xdisplay, r->TermWin.gc, r->TermWin.font->fid);
	    *draw_string = XDrawString;
	    *draw_image_string = XDrawImageString;
	}
#endif /* MULTICHAR_SET */
	{
	    /* mmc: i do assume, that i don't use proportional fonts! */
	    if (!fprop) {
		/* aaaabbb
		 * acaacxx  so at c we detect first change.
		 *    ^ here our patience runs off.
		 * then we walk and as much as ....even when identical
		 * but only half at most. */
		int trailing_redundants;  /* # of redundant/useless  */
		/* single stepping - `normal' mode */
		for (trailing_redundants = 0; ++col < r->TermWin.ncol - 1;){
		    /* if the font changes */
		    if (rend != srp[col])
			break;
		    /* mmc: do we reprocess the starting [0] ?? */
		    buffer[len++] = stp[col];

		    /* if even this is different: */
		    if ((stp[col] != dtp[col]) || (srp[col] != drp[col])) {
			/* fixme: must_clear is FALSE! */
			if (must_clear && (trailing_redundants++ > (len / 2)))
			    break;

			/* mmc: Soft Rendering:	 moving from Buffer to Display (buffer) */
			dtp[col] = stp[col];
			drp[col] = srp[col];
			/* reset i */
			trailing_redundants = 0;
		    } else if (must_clear || (stp[col] != ' ' && ++trailing_redundants > 32))
			break;
		}

		col--;	/* went one too far.  move back */
		len -= trailing_redundants; /* dump any matching trailing chars */
	    }
	    *wlen = len;
	}
	buffer[len] = '\0';
#ifdef MULTICHAR_SET
    }
#endif

    *ret_col = col;
    *ret_len = len;
}

inline void
rewrite_buffer(char buffer[], rend_t rend, int16_t len)
{
    switch (rend & RS_fontMask) {
	case RS_acsFont:
	{
	    int i;
	    for (i = 0; i < len; i++)
		if (buffer[i] == 0x5f)
		    buffer[i] = 0x7f;
		else if (buffer[i] > 0x5f && buffer[i] < 0x7f)
		    buffer[i] -= 0x5f;
	    break;
	}
	case RS_ukFont:
	{
	    int i;
	    for (i = 0; i < len; i++)
		if (buffer[i] == '#')
		    buffer[i] = 0x1e;	/* pound sign */
	    break;
	}
    }
}

/* given the attributes of the cell + global ones -> find the combination?
   Also -- side effect --- sets the X GC of the @r@ */
// fixme: would be better to use abstract type!
/* inline */ void
determine_attributes(const rxvt_t *r,
		     rend_t	     rend,
#ifdef TTY_256COLOR
		     u_int16_t	    fore,
		     u_int16_t	    back,	/* desired foreground/background	     */
#else
		     unsigned char   fore,
		     unsigned char   back,	/* desired foreground/background	     */
#endif
		     unsigned char  *bfont,
		     unsigned long   *ret_gcmask,	 /* Graphics Context mask		 */
		     XGCValues	     *gcvalue,
		     unsigned char wbyte,
		     unsigned char   *fontdiff
		     )
{
    unsigned long   gcmask;
    unsigned char rvid;	/* reverse video this position */
    struct rxvt_hidden *h = r->h;

    rvid = (rend & RS_RVid) ? 1 : 0;
#ifdef OPTION_HC
    if (!rvid && (rend & RS_Blink)) {
	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_HC)
	    && r->PixColors[fore] != r->PixColors[Color_HC]
	    && r->PixColors[back] != r->PixColors[Color_HC])
	    back = Color_HC;
	else
	    rvid = !rvid;	/* fall back */
    }

#endif
    if (rvid) {
#ifdef TTY_256COLOR
	SWAP_IT(fore, back, u_int16_t);
#else
	SWAP_IT(fore, back, unsigned char);
#endif
#ifndef NO_BOLD_UNDERLINE_REVERSE
	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_RV)
	    && r->PixColors[fore] != r->PixColors[Color_RV])
	    back = Color_RV;
#endif
    }
    gcmask = 0;
    if (back != Color_bg) {
	gcvalue->background = r->PixColors[back];
	gcmask = GCBackground;
    }
    if (fore != Color_fg) {
	gcvalue->foreground = r->PixColors[fore];
	gcmask |= GCForeground;
    }
#ifndef NO_BOLD_UNDERLINE_REVERSE
    else if (rend & RS_Bold) {
	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_BD)
	    && r->PixColors[fore] != r->PixColors[Color_BD]
	    && r->PixColors[back] != r->PixColors[Color_BD]) {
	    gcvalue->foreground = r->PixColors[Color_BD];
	    gcmask |= GCForeground;
# ifndef VERYBOLD
	    rend &= ~RS_Bold;	/* we've taken care of it */
# endif
	}
    } else if (rend & RS_Uline) {
	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_UL)
	    && r->PixColors[fore] != r->PixColors[Color_UL]
	    && r->PixColors[back] != r->PixColors[Color_UL]) {
	    gcvalue->foreground = r->PixColors[Color_UL];
	    gcmask |= GCForeground;
	    rend &= ~RS_Uline;	/* we've taken care of it */
	}
    }
#endif
    if (gcmask)
	XChangeGC(r->Xdisplay, r->TermWin.gc, gcmask, gcvalue);
#ifndef NO_BOLDFONT
    if (!wbyte && MONO_BOLD_FG(rend, fore)
	&& r->TermWin.boldFont != NULL) {
	*bfont = 1;
	XSetFont(r->Xdisplay, r->TermWin.gc, r->TermWin.boldFont->fid);
	*fontdiff = (r->TermWin.propfont & PROPFONT_BOLD);
	rend &= ~RS_Bold;	/* we've taken care of it */
    } else if (*bfont) {
	*bfont = 0;
	XSetFont(r->Xdisplay, r->TermWin.gc, r->TermWin.font->fid);
    }
#endif

    *ret_gcmask = gcmask;
}


/*
 * Actually do the drawing of the string here
 */
inline void
do_draw(rxvt_t *r, rend_t rend,
	char buffer[], int16_t len, int16_t wlen,
	int xpixel, int ypixel, int ypixelc,
#ifdef TTY_256COLOR
	u_int16_t	fore,
	u_int16_t	back,	/* desired foreground/background	     */
#else
	unsigned char	fore,
	unsigned char	back,	/* desired foreground/background	     */
#endif
	unsigned char wbyte, unsigned char must_clear,
	XGCValues	*gcvalue,
	unsigned long	gcmask,
	unsigned char	fontdiff,
	unsigned char	fprop,
	draw_function  draw_string,
	draw_function  draw_image_string
	)
{
    /* draw_string */

    /* mmc: what is needed from previous computation:
     *
     *	 back ?
     *	 must_clear ?
     *	 ---------
     *	 fprop	      proportional font used
     *	 fontdiff     current font size != base font size
     * */
    if (back == Color_bg && must_clear) {
	/* mmc: we can simply XClearRectangle and then draw the string (foreground) */

	D_SCREEN((stderr, "%s CLEAR_CHARS: (%d, %d -- %d)\n",
		  __FUNCTION__,xpixel, ypixelc, len));

	CLEAR_CHARS(xpixel, ypixelc, len);
	int i;
	for (i = 0; i < len; i++)	/* don't draw empty strings */
	    if (buffer[i] != ' ') {
		DRAW_STRING(draw_string, xpixel, ypixel, buffer, wlen);
		break;
	    }
    } else if (fprop || fontdiff) {	/* single glyph writing */
	unsigned long	gctmp;

	gctmp = gcvalue->foreground;
	gcvalue->foreground = gcvalue->background;
	XChangeGC(r->Xdisplay, r->TermWin.gc, GCForeground, gcvalue);

	D_SCREEN((stderr, "%s XFillRectangle: (%d, %d -- %d, %d)\n",
		  __FUNCTION__,
		  xpixel, ypixelc,
		  Width2Pixel(len), (Height2Pixel(1) - r->TermWin.lineSpace)));

	/* |XX....X|  len characters, (on 1 line)
	 *  fixme:
	 *  mmc: we don't use XClearAres (CLEAR_CHARS) b/c that would put the (original) Window BG
	 *  colors.  Here we _probably_ need a different color background! (selected by ANSII
	 *  color sequence) */
	XFillRectangle(r->Xdisplay, drawBuffer, r->TermWin.gc,
		       xpixel, ypixelc, (unsigned int)Width2Pixel(len),
		       (unsigned int)(Height2Pixel(1)
				      - r->TermWin.lineSpace));
	/* mmc:	 TermWin.lineSpace   is the space between 2 lines? */
	gcvalue->foreground = gctmp;
	XChangeGC(r->Xdisplay, r->TermWin.gc, GCForeground, gcvalue);
	DRAW_STRING(draw_string, xpixel, ypixel, buffer, wlen);
	/* mmc: note where we draw! ypixel ...font line
	 * and ypixelc is bottom of the cell! */
    } else {
	/* D_SCREEN((stderr, "%s just DRAW_STRING:", __FUNCTION__));*/
	DRAW_STRING(draw_image_string, xpixel, ypixel, buffer, wlen);
    }
#ifndef NO_BOLDOVERSTRIKE
# ifdef NO_BOLDOVERSTRIKE_MULTI
    if (!wbyte)
# endif
	if (MONO_BOLD_FG(rend, fore))
	    DRAW_STRING(draw_string, xpixel + 1, ypixel, buffer, wlen);
#endif

    /* Underlining! */
    if ((rend & RS_Uline) && (r->TermWin.font->descent > 1))
	XDrawLine(r->Xdisplay, drawBuffer, r->TermWin.gc,
		  xpixel, ypixel + 1,
		  xpixel + Width2Pixel(len) - 1, ypixel + 1);
    /* restore normal colours */
    if (gcmask) {
	gcvalue->foreground = r->PixColors[Color_fg];
	gcvalue->background = r->PixColors[Color_bg];
	XChangeGC(r->Xdisplay, r->TermWin.gc, gcmask, gcvalue);
    }
}

/**
   Comparing 2 matrics. What should be and what is on the window surface.
   SCREEN(desired) is the active one of the R. Either PRIMARY or SECONDARY!

   screen+row_offset	   r->drawn_text
   xxx
   yyy - row_offset   vs   jjjjj
   zzz			   kkkkk
   versus  limited to top_row .... bottom_row
   (exclusive!)

   mmc: todo: [02 nov 06] accept another parameter: `yoffset' so that we can draw in
   micro-scrolled window!
*/

/* INTPROTO */
void
redraw_matrix(rxvt_t *r,screen_t *screen, int row_offset, int top_row, int bottom_row,
	      unsigned char *clearfirst, unsigned char *clearlast, int yoffset)
{
    int16_t	    col, row;	/* column/row we're processing		     */
    text_t	   *dtp, *stp;	/* drawn-text-pointer, screen-text-pointer   */
    rend_t	   *drp, *srp;	/* drawn-rend-pointer, screen-rend-pointer   */
    int16_t	    len, wlen;	/* text length screen/buffer		     */
    char	   *buffer;	/* local copy of r->h->buffer		     */
    unsigned char    must_clear,/* must cover/repaint the whole rectangle, i.e.
				   use draw_string not draw_image_string     */
#ifndef NO_BOLDFONT
		    bfont = 0,	/* we've changed font to bold font	     */
#endif
		    wbyte;	/* we're in multibyte			     */

/* mmc: todo: have a special type! and avoid #ifdef! */
#ifdef TTY_256COLOR
    u_int16_t	    fore, back;	/* desired foreground/background	     */
#else
    unsigned char   fore, back;	/* desired foreground/background	     */
#endif
    struct rxvt_hidden *h = r->h;
    int		     total_drawn = 0;
    /* mmc: I might replace these */
    int		    (*draw_string) () = XDrawString;
    int		    (*draw_image_string) () = XDrawImageString;

    XGCValues	    gcvalue;	/* Graphics Context values		     */
    /* set base colours to avoid check in "single glyph writing" below */
    gcvalue.foreground = r->PixColors[Color_fg];
    gcvalue.background = r->PixColors[Color_bg];

    /* ->buffer is used only for redrawing. */
    must_clear = wbyte = 0;
    if (h->currmaxcol < r->TermWin.ncol) {
	h->currmaxcol = r->TermWin.ncol;
	h->buffer = rxvt_realloc(h->buffer, sizeof(char) * (h->currmaxcol + 1));
    }
    buffer = h->buffer;

    if (r->TermWin.saveLines + r->TermWin.nrow < bottom_row)
	fprintf(stderr, "%s: problem here!\n", __FUNCTION__);

    /*
     * E: main pass across every character
     */
    for (row = top_row; row < bottom_row; row++) { /* fixme:  should take <= bottom_row ? */
	int		xpixel,	 /* x offset for start of drawing (font) */
			ypixel,	 /* y offset for start of drawing (font) */
			ypixelc; /* y offset for top of drawing		 */
	unsigned long	gcmask;	 /* Graphics Context mask		 */

	/* fixme: these are out of bound! */
	stp = screen->text[row + row_offset];
	srp = screen->rend[row + row_offset];
	dtp = r->drawn_text[row];
	drp = r->drawn_rend[row];

	/* E1: */
#ifndef NO_PIXEL_DROPPING_AVOIDANCE
	mark_damaged_chars_from_left(r, stp, srp, dtp, drp,
				     clearfirst, clearlast);
#endif /* NO_PIXEL_DROPPING_AVOIDANCE */

	/*
	 * E2: OK, now the real pass
	 */
	ypixelc = (int)Row2Pixel(row) + yoffset;
	ypixel = ypixelc + r->TermWin.font->ascent;

	for (col = 0; col < r->TermWin.ncol; col++){
	    unsigned char   fontdiff,/* current font size != base font size */
			    fprop;   /* proportional font used		    */
	    rend_t	    rend;    /* rendition value			    */

	    rend = srp[col];
	    /* compare new text with old - if exactly the same then continue */
	    if (cell_stays_unchanged(rend, &col,
				     stp, dtp, drp))
		continue;

	    /* redraw one or more characters */
	    fontdiff = 0;
	    len = 0;

	    /* mmc:   Here we update the  drawn Buffer (soft screen) !	*/
	    buffer[len++] = dtp[col] = stp[col]; /* mmc: we start to construct the string to be drawn */
	    drp[col] = rend;	/* and update the drawn (buffer). */
	    xpixel = Col2Pixel(col);

	    /*
	     * Find out the longest string we can write out at once
	     */
#ifndef NO_BOLDFONT
	    if (MONO_BOLD(rend) && r->TermWin.boldFont != NULL)
		fprop = (r->TermWin.propfont & PROPFONT_BOLD);
	    else
#endif
		fprop = (r->TermWin.propfont & PROPFONT_NORMAL);

	    find_longest_string_to_draw(r, rend,
					fprop, &len, &wlen, buffer,
					&col, &fontdiff,
					stp,
					srp,
					dtp,
					drp,
					&draw_string,
					&draw_image_string,
					&wbyte,
					must_clear);
	    total_drawn += len;
	    /* so in buffer[] we have the text to print. */

	    /*
	     * Determine the attributes for the string
	     */
	    fore = GET_FGCOLOR(rend);
	    back = GET_BGCOLOR(rend);
	    rend = GET_ATTR(rend);

	    rewrite_buffer(buffer, rend, len);

	    determine_attributes(r, rend,
				 fore, back,
				 &bfont, &gcmask,
				 &gcvalue,wbyte, &fontdiff);
	    do_draw(r, rend,
		    buffer, len, wlen,
		    xpixel, ypixel, ypixelc,
		    back, fore,
		    wbyte, must_clear, &gcvalue, gcmask,
		    fontdiff, fprop,

		    draw_string, draw_image_string);
	}			/* for (col....) */
    }				/* for (row....) */

    if (r->h->debug & 1)
	fprintf(stderr, "total %d\n", total_drawn);
}


/* Sets in the desired `screen' the rendition of the cursor (if the X window is focused).
 * Also, sets   h->oldcursor */

/* main_screen is used to get all info, apart from the text/render buffer.
 * That is taken from screen. */
/* INTPROTO */
void
set_cursor_characters(rxvt_t *r,screen_t *main_screen, screen_t *screen,
		      rend_t* cc1, rend_t* cc2, int16_t ocrow, char *morecur)
{
    /* we'll use some aliases */
    struct rxvt_hidden *h = r->h;
    rend_t	   *srp;	/* screen-rend-pointer, drawn-rend-pointer   */

    unsigned char   setoldcursor;
    rend_t	    ccol1,	/* Cursor colour       */
		    ccol2;	/* Cursor colour2      */


    if ((main_screen->flags & Screen_VisibleCursor) && r->TermWin.focus) {
	srp = &(screen->rend[main_screen->cur.row + r->TermWin.saveLines]
			    [main_screen->cur.col]);
	*srp ^= RS_RVid;	/* XOR the Reverse Video Bit */

	/* mmc: cursor uses a special color, not simply reverse: */
#ifndef NO_CURSORCOLOR
	*cc1 = *srp & (RS_fgMask | RS_bgMask);
	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_cursor))
	    ccol1 = Color_cursor;
	else {
#ifdef CURSOR_COLOR_IS_RENDITION_COLOR
	    ccol1 = GET_FGCOLOR(h->rstyle);
#else
	    ccol1 = Color_fg;
#endif
	}

	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_cursor))
	    ccol2 = Color_cursor2;
	else {
#ifdef CURSOR_COLOR_IS_RENDITION_COLOR
	    ccol2 = GET_BGCOLOR(h->rstyle);
#else
	    ccol2 = Color_bg;
#endif
	}
	*srp = SET_FGCOLOR(*srp, ccol1);
	*srp = SET_BGCOLOR(*srp, ccol2);
#endif	/* NO_CURSORCOLOR */
#ifdef MULTICHAR_SET
	if (IS_MULTI1(*srp)) {
	    if (main_screen->cur.col < r->TermWin.ncol - 2
		&& IS_MULTI2(*++srp))
		*morecur = 1;
	} else if (IS_MULTI2(*srp)) {
	    if (main_screen->cur.col > 0 && IS_MULTI1(*--srp))
		*morecur = -1;
	}
	if (*morecur) {
	    *srp ^= RS_RVid;
# ifndef NO_CURSORCOLOR
	    *cc2 = *srp & (RS_fgMask | RS_bgMask);
	    *srp = SET_FGCOLOR(*srp, ccol1);
	    *srp = SET_BGCOLOR(*srp, ccol2);
# endif
	}
#endif	/* MULTICHAR_SET */
    }
    /* make sure no outline cursor is left around */
    setoldcursor = 0;
    if (ocrow != -1) {
	if (main_screen->cur.row + r->TermWin.view_start != ocrow
	    || main_screen->cur.col != h->oldcursor.col) {
	    if (ocrow < r->TermWin.nrow
		&& h->oldcursor.col < r->TermWin.ncol) {
		r->drawn_rend[ocrow][h->oldcursor.col] ^= (RS_RVid | RS_Uline);
#ifdef MULTICHAR_SET
		if (h->oldcursormulti) {
		    int col;
		    col = h->oldcursor.col + h->oldcursormulti;
		    if (col < r->TermWin.ncol)
			r->drawn_rend[ocrow][col] ^= (RS_RVid | RS_Uline);
		}
#endif
	    }
	    if (r->TermWin.focus
		|| !(main_screen->flags & Screen_VisibleCursor))
		h->oldcursor.row = -1;
	    else
		setoldcursor = 1;
	}
    } else if (!r->TermWin.focus)
	setoldcursor = 1;

    if (setoldcursor) {
	if (main_screen->cur.row + r->TermWin.view_start >= r->TermWin.nrow)
	    h->oldcursor.row = -1;
	else {
	    h->oldcursor.row = main_screen->cur.row + r->TermWin.view_start;
	    h->oldcursor.col = main_screen->cur.col;
#ifdef MULTICHAR_SET
	    h->oldcursormulti = *morecur;
#endif
	}
    }
}

#ifndef NO_SLOW_LINK_SUPPORT
/* mmc: we do support slow link: so: */

/* INTPROTO */
unsigned char
try_to_scroll(rxvt_t *r, unsigned char refresh_type, screen_t *screen,
	      unsigned char must_clear, int row_offset, int16_t ocrow)
{
    /*
     * D: CopyArea pass - very useful for slower links
     *	  This has been deliberately kept simple.
     */

    /* adding: */
    struct rxvt_hidden *h = r->h;
    const int i = h->num_scr;		  /* mmc: ->num_scr  is the scrolling amount? */;

    /* mmc: */
    if (i)
	refresh_type = FAST_REFRESH;

    D_SCREEN((stderr, "rxvt_scr_refresh(): scroll %d, refresh: %s (%s) %s",
	      i,(refresh_type == FAST_REFRESH)?"FAST_REFRESH":
	      (refresh_type == SLOW_REFRESH)?"SLOW_REFRESH":"DUNNO",
	      (h->num_scr_allow?"":"not allow"),
	      (must_clear)?"MUST_CLEAR":""));

    /* mmc: This is for scrolling! */
    if (refresh_type == FAST_REFRESH && h->num_scr_allow && i
	/* mmc:	 FAST_REFRESH  REFRESH_BOUNDS */
	&& abs(i) < r->TermWin.nrow && !must_clear) {

	int16_t		len, wlen;	/* text length screen/buffer		     */
	int16_t		col, row;	/* column/row we're processing		     */

	text_t	       *dtp, *stp;	/* drawn-text-pointer, screen-text-pointer   */
	rend_t	       *drp, *srp;	/* drawn-rend-pointer, screen-rend-pointer   */


	int16_t		nits;
	int		j;
	rend_t	       *drp2;
	text_t	       *dtp2;

	D_SCREEN((stderr, "rxvt_scr_refresh(): D: XCopyArea pass"));
	j = r->TermWin.nrow;	/* ?? */
	wlen = len = -1;

	/* i is the direction? */
	row = i > 0 ? 0 : j - 1;

	for (; j-- >= 0; row += (i > 0 ? 1 : -1)) {

	    /* mmc:  the row will be scrolled into visible area?     todo: what is ocrow ? */
	    if (row + i >= 0 && row + i < r->TermWin.nrow && row + i != ocrow) {

		/* mmc: `screen'  TEXT (strings) and  RENDERED (colors & attributes ??)	 */
		stp = screen->text[row + row_offset];
		/* row_offset is for displaying	  `scroll_back': history! */
		srp = screen->rend[row + row_offset]; /* so, compare it with DTP, not DTP2 */


		/* `Drawn'    from To:	*/
		dtp = r->drawn_text[row];
		dtp2 = r->drawn_text[row + i];

		drp = r->drawn_rend[row];
		drp2 = r->drawn_rend[row + i];


		/*  */
		for (nits = 0, col = r->TermWin.ncol; col--; )

		    /* we compare the buffer, with the 2 displayed rows. Which one is more
		       similar?*/

		    /* Change between display, and buffer: */
		    /* where it would be if we moved */
		    if (stp[col] != dtp2[col] || srp[col] != drp2[col])
			nits--;

		    /* Where it is now: */
		    else if (stp[col] != dtp[col] || srp[col] != drp[col])
			nits++;
		/* The displayed row is different from the buffer! not useful to accelerate the move. */


		/* useful to move this line:
		 * we have 2 levels:
		 *
		 *  |	     |
		 *  |---len--|
		 *  |	     |--+
		 *  |--wlen--|	|
		 *  |	     |	|  accelerated move
		 *  |	     |	|
		 *  ---------<--+
		 *
		 * */
		/* mmc: changing 8 -> 0 */
		if (nits > 0) {	/* XXX: arbitrary choice */
		    /* move in memory: */
		    for (col = r->TermWin.ncol; col--; ) {
			*dtp++ = *dtp2++;
			*drp++ = *drp2++;
		    }
		    /* start of the region:  */
		    if (len == -1)
			len = row;
		    /* mmc: push the new end: */
		    wlen = row;
		    continue;	/* mmc: So we skip the XCopyArea, and will do it in next cycle! */
		    /* bug: What if continue will exit the cycle!!! */
		}
	    }

	    if (len != -1) {
		/* also comes here at end if needed because of >= above */
		if (wlen < len)
		    SWAP_IT(wlen, len, int);
		/* D_SCREEN */
#if mmc_debug
		if (h->debug)
			fprintf(stderr, "%srxvt_scr_refresh(): XCopyArea: "
				"moving line %d -> %d by %d (height of the moving block %d)%s\n",
				color_red,
				len, wlen, i,
				wlen - len + 1,
				color_reset);
#endif
		XCopyArea(r->Xdisplay, drawBuffer, drawBuffer,
			  r->TermWin.gc,
			  /* start */
			  0, Row2Pixel(len + i),
			  /* dimension: */
			  (unsigned int)TermWin_TotalWidth(),
			  (unsigned int)Height2Pixel(wlen - len + 1),
			  /* destination */
			  0, Row2Pixel(len));
		len = -1;
	    }
	}
	if (len != -1)
	    fprintf(stderr,"BUG\n");

    }
    return refresh_type;
}
#endif /* NO_SLOW_LINK_SUPPORT */

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

/* mmc: my hack to implement frozen snapshot, but keeping
 *     rxvt_scr_refresh
 * all primary & secondary screen processing unchanged.
 * Improvement would be to use only the snapshot in `rxvt_scr_refresh' !
 */

/* INTPROTO */
void
exchange_snapshot_screen(rxvt_t *r, int offset)
{
#if 0
    /* ugly hack, sorry */
    int offset = r->TermWin.saveLines; /* (mmc:) number of lines that fit in scrollback */
#endif
    int i;
    if (r->h->prev_nrow != r->TermWin.nrow)
	{
	    fprintf(stderr, "%s: %sbug%s: %d %d\n",__FUNCTION__,
		    color_green, color_reset,
		    r->h->prev_nrow, r->TermWin.nrow);
	    abort();
	}

    for (i = r->TermWin.nrow; i--;) { /* fixme: why prev_nrow?? this is nonsense! */
	if (!r->snapshot.rend[i])
	    {
		fprintf(stderr, "%s: %sbug%s: line %d in snapshot is NULL\n",__FUNCTION__,
			color_green, color_reset, i);
		abort();
	    }


	SWAP_IT(r->screen.tlen[i + offset], r->snapshot.tlen[i], int16_t);
	SWAP_IT(r->screen.text[i + offset], r->snapshot.text[i], text_t *);
	SWAP_IT(r->screen.rend[i + offset], r->snapshot.rend[i], rend_t *);
    }

    // CHECK_CONSISTENCY(r, __FUNCTION__,0);
}
