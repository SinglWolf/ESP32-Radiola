/*
  ucg_dev_ic_st7735.c
  
  Specific code for the st7735 controller (TFT displays)
  Universal uC Color Graphics Library
  
  Copyright (c) 2014, olikraus@gmail.com
  All rights reserved.
  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
*/

#include "ucg.h"


const ucg_pgm_uint8_t ucg_st7735_set_pos_seqw[] = // init rotate
{
  UCG_CS(0),					/* enable chip */
  UCG_C11( 0x036, 0x000),
  UCG_C10(0x02a),	UCG_VARX(0,0x00, 0), UCG_VARX(0,0x0ff, 0), UCG_A2(0x000, 0x07f+2),		/* set x position */
  UCG_C10(0x02b),	UCG_VARY(0,0x00, 0), UCG_VARY(0,0x0ff, 0), UCG_A2(0x000, 0x09f+1),		/* set y position */
  UCG_C10(0x02c),							/* write to RAM */
  UCG_DATA(),								/* change to data mode */
  UCG_END()
};


const ucg_pgm_uint8_t ucg_st7735_set_pos_dir0_seqw[] = // unrotate
{
  UCG_CS(0),					/* enable chip */
  
  /* 0x000 horizontal increment (dir = 0) */
  /* 0x000 vertical increment (dir = 1) */
  /* 0x040 horizontal deccrement (dir = 2) */
  /* 0x080 vertical deccrement (dir = 3) */
  UCG_C11( 0x036, 0x000),
  UCG_C10(0x02a),	UCG_VARX(0,0x00, 0), UCG_VARX(0,0x0ff, 0), UCG_A2(0x000, 0x07f+2),		/* set x position */
  UCG_C10(0x02b),	UCG_VARY(0,0x00, 0), UCG_VARY(0,0x0ff, 0), UCG_A2(0x000, 0x09f+1),		/* set y position */

  UCG_C10(0x02c),							/* write to RAM */
  UCG_DATA(),								/* change to data mode */
  UCG_END()
};

const ucg_pgm_uint8_t ucg_st7735_set_pos_dir1_seqw[] = // rotate90
{
  UCG_CS(0),					/* enable chip */
  /* 0x000 horizontal increment (dir = 0) */
  /* 0x000 vertical increment (dir = 1) */
  /* 0x040 horizontal deccrement (dir = 2) */
  /* 0x080 vertical deccrement (dir = 3) */
  UCG_C11( 0x036, 0x000),
  UCG_C10(0x02a),	UCG_VARX(0,0x00, 0), UCG_VARX(0,0x0ff, 0), UCG_VARX(0,0x00, 0), UCG_VARX(0,0x0ff, 0),	/* set x position */
  UCG_C10(0x02b),	UCG_VARY(0,0x00, 0), UCG_VARY(0,0x0ff, 0), UCG_A2(0x000, 0x09f+1),						/* set y position */

  UCG_C10(0x02c),							/* write to RAM */
  UCG_DATA(),								/* change to data mode */
  UCG_END()
};

const ucg_pgm_uint8_t ucg_st7735_set_pos_dir2_seqw[] = //rotate180
{
  UCG_CS(0),					/* enable chip */
  
  /* 0x000 horizontal increment (dir = 0) */
  /* 0x000 vertical increment (dir = 1) */
  /* 0x040 horizontal deccrement (dir = 2) */
  /* 0x080 vertical deccrement (dir = 3) */
  
  UCG_C11( 0x036, 0x040),
  //UCG_C11( 0x036, 0x040),			/* it seems that this command needs to be sent twice */
  UCG_C10(0x02a),	UCG_VARX(0,0x00, 0), UCG_VARX(0,0x0ff, 0), UCG_A2(0x000, 0x07f+2),					/* set x position */
  UCG_C10(0x02b),	UCG_VARY(0,0x00, 0), UCG_VARY(0,0x0ff, 0), UCG_A2(0x000, 0x09f+1),		/* set y position */

  UCG_C10(0x02c),							/* write to RAM */
  UCG_DATA(),								/* change to data mode */
  UCG_END()
};

const ucg_pgm_uint8_t ucg_st7735_set_pos_dir3_seqw[] = //rotate270
{
  UCG_CS(0),					/* enable chip */
  
  /* 0x000 horizontal increment (dir = 0) */
  /* 0x000 vertical increment (dir = 1) */
  /* 0x0c0 horizontal deccrement (dir = 2) */
  /* 0x0c0 vertical deccrement (dir = 3) */
  UCG_C11( 0x036, 0x080),
  UCG_C11( 0x036, 0x080),		/* it seems that this command needs to be sent twice */
  UCG_C10(0x02a),	UCG_VARX(0,0x00, 0), UCG_VARX(0,0x0ff, 0), UCG_VARX(0,0x00, 0), UCG_VARX(0,0x0ff, 0),					/* set x position */
  UCG_C10(0x02b),	UCG_VARY(0,0x00, 0), UCG_VARY(0,0x0ff, 0), UCG_A2(0x000, 0x09f+1),		/* set y position */

  UCG_C10(0x02c),							/* write to RAM */
  UCG_DATA(),								/* change to data mode */
  UCG_END()
};

ucg_int_t ucg_handle_st7735_l90fxw(ucg_t *ucg)
{
  uint8_t c[3];
  ucg_int_t tmp;

  if ( ucg_clip_l90fx(ucg) != 0 )
  {
    switch(ucg->arg.dir)
    {
      case 0: 			// undo rotate?
		ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_dir0_seqw);	
		break;
      case 1: 			// rotate90??
		ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_dir1_seqw);	
		break;
      case 2:			// rotate180 (drawFrame, )
		tmp = ucg->arg.pixel.pos.x ;
		ucg->arg.pixel.pos.x = (127)-tmp;
		ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_dir2_seqw);	
		ucg->arg.pixel.pos.x = tmp;
		break;
      case 3: 			// rotate270
      default: 
		tmp = ucg->arg.pixel.pos.y;
		ucg->arg.pixel.pos.y = (159)-tmp;
		ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_dir3_seqw);	
		ucg->arg.pixel.pos.y = tmp;
		break;
    }
	c[0] = ucg->arg.pixel.rgb.color[0];
	c[1] = ucg->arg.pixel.rgb.color[1];
    c[2] = ucg->arg.pixel.rgb.color[2];
    ucg_com_SendRepeat3Bytes(ucg, ucg->arg.len, c);
    ucg_com_SetCSLineStatus(ucg, 1);		/* disable chip */
    return 1;
  }
  return 0;
}

/*
  L2TC (Glyph Output)
  
*/

/* with CmdDataSequence */ 
ucg_int_t ucg_handle_st7735_l90tcw(ucg_t *ucg)
{	
  if ( ucg_clip_l90tc(ucg) != 0 )
  {
    uint8_t buf[16];
    ucg_int_t dx, dy;
    ucg_int_t i;
    unsigned char pixmap;
    uint8_t bitcnt;
    ucg_com_SetCSLineStatus(ucg, 0);		/* enable chip */
    ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_seqw);	

    buf[0] = 0x001;	// change to 0 (cmd mode)
    buf[1] = 0x02a;	// set x
    buf[2] = 0x002;	// change to 1 (arg mode)
    buf[3] = 0x000;	// upper part x
    buf[4] = 0x000;	// no change
    buf[5] = 0x000;	// will be overwritten by x value
    buf[6] = 0x001;	// change to 0 (cmd mode)
    buf[7] = 0x02c;	// write data
    buf[8] = 0x002;	// change to 1 (data mode)
    buf[9] = 0x000;	// red value
    buf[10] = 0x000;	// no change
    buf[11] = 0x000;	// green value
    buf[12] = 0x000;	// no change
    buf[13] = 0x000;	// blue value      
    
    switch(ucg->arg.dir)
    {
      case 0: 
		dx = 1; dy = 0; 
		buf[1] = 0x02a;	// set x
		break;
      case 1: 	
		dx = 0; dy = 1; 
        buf[1] = 0x02b;	// set y
		break;
      case 2: 
		dx = -1; dy = 0; 
        buf[1] = 0x02a;	// set x
		break;
      case 3: 
      default:
		dx = 0; dy = -1; 
        buf[1] = 0x02b;	// set y
		break;
    }
	pixmap = ucg_pgm_read(ucg->arg.bitmap);
    bitcnt = ucg->arg.pixel_skip;
    pixmap <<= bitcnt;
    buf[9] = ucg->arg.pixel.rgb.color[0];
    buf[11] = ucg->arg.pixel.rgb.color[1];
    buf[13] = ucg->arg.pixel.rgb.color[2];
    //ucg_com_SetCSLineStatus(ucg, 0);		/* enable chip */
    
    for( i = 0; i < ucg->arg.len; i++ )
    {
      if ( (pixmap & 128) != 0 )
      {
	    if ( (ucg->arg.dir&1) == 0 )
		{
			buf[5] = ucg->arg.pixel.pos.x;
		}
		else
		{
			buf[3] = ucg->arg.pixel.pos.y>>8;
			buf[5] = ucg->arg.pixel.pos.y&255;
		}
		ucg_com_SendCmdDataSequence(ucg, 7, buf, 0);
      }
      pixmap<<=1;
      ucg->arg.pixel.pos.x+=dx;
      ucg->arg.pixel.pos.y+=dy;
      bitcnt++;
      if ( bitcnt >= 8 )
      {
		ucg->arg.bitmap++;
		pixmap = ucg_pgm_read(ucg->arg.bitmap);
		bitcnt = 0;
      }
    }
    ucg_com_SetCSLineStatus(ucg, 1);		/* disable chip */
    return 1;
  }
  return 0;
}


ucg_int_t ucg_handle_st7735_l90sew(ucg_t *ucg)
{
  uint8_t i;
  uint8_t c[3];
  ucg_int_t tmp;
  
  /* Setup ccs for l90se. This will be updated by ucg_clip_l90se if required */
  for ( i = 0; i < 3; i++ )
  {
    ucg_ccs_init(ucg->arg.ccs_line+i, ucg->arg.rgb[0].color[i], ucg->arg.rgb[1].color[i], ucg->arg.len);
  }
  
  /* check if the line is visible */
  
  if ( ucg_clip_l90se(ucg) != 0 ){
	ucg_int_t i;
    switch(ucg->arg.dir){
		case 0: 
			ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_dir0_seqw);	
		break;
		case 1: 
			ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_dir1_seqw);	
			break;
		case 2: 
			tmp = ucg->arg.pixel.pos.x;
			ucg->arg.pixel.pos.x = (127)-tmp;
			ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_dir2_seqw);	
			ucg->arg.pixel.pos.x = tmp;
			break;
		case 3: 
		default: 
			tmp = ucg->arg.pixel.pos.y;
			ucg->arg.pixel.pos.y = (159)-tmp;
			ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_dir3_seqw);	
			ucg->arg.pixel.pos.y = tmp;
			break;
	}
    
    for( i = 0; i < ucg->arg.len; i++ )
    {
      c[0] = ucg->arg.ccs_line[0].current;
      c[1] = ucg->arg.ccs_line[1].current; 
      c[2] = ucg->arg.ccs_line[2].current;
      ucg_com_SendRepeat3Bytes(ucg, 1, c);
      ucg_ccs_step(ucg->arg.ccs_line+0);
      ucg_ccs_step(ucg->arg.ccs_line+1);
      ucg_ccs_step(ucg->arg.ccs_line+2);
    }
    ucg_com_SetCSLineStatus(ucg, 1);		/* disable chip */
    return 1;
  }
  return 0;
}


static const ucg_pgm_uint8_t ucg_st7735_power_down_seqw[] = {
	UCG_CS(0),					/* enable chip */
	UCG_C10(0x010),				/* sleep in */
	UCG_C10(0x28), 				/* display off */	
	UCG_CS(1),					/* disable chip */
	UCG_END(),					/* end of sequence */
};

ucg_int_t ucg_dev_ic_st7735W_18(ucg_t *ucg, ucg_int_t msg, void *data){
	switch(msg){
		case UCG_MSG_DEV_POWER_UP:
			/* setup com interface and provide information on the clock speed */
			/* of the serial and parallel interface. Values are nanoseconds. */
			ucg->display_offset.x = 2;
			ucg->display_offset.y = 1;
			return ucg_com_PowerUp(ucg, 80, 66);
		case UCG_MSG_DEV_POWER_DOWN:
			ucg_com_SendCmdSeq(ucg, ucg_st7735_power_down_seqw);
			return 1;
		case UCG_MSG_GET_DIMENSION:
			((ucg_wh_t *)data)->w = 128;
			((ucg_wh_t *)data)->h = 160;
			return 1;
		case UCG_MSG_DRAW_PIXEL:
			if ( ucg_clip_is_pixel_visible(ucg) !=0 ){
			uint8_t c[3];
			ucg_com_SendCmdSeq(ucg, ucg_st7735_set_pos_seqw);	
			c[0] = ucg->arg.pixel.rgb.color[0];
			c[1] = ucg->arg.pixel.rgb.color[1];
			c[2] = ucg->arg.pixel.rgb.color[2];
			ucg_com_SendRepeat3Bytes(ucg, 1, c);
			ucg_com_SetCSLineStatus(ucg, 1);		/* disable chip */
			}
			return 1;
		case UCG_MSG_DRAW_L90FX:
			//ucg_handle_l90fx(ucg, ucg_dev_ic_st7735_18);
			ucg_handle_st7735_l90fxw(ucg);
			return 1;
#ifdef UCG_MSG_DRAW_L90TC
		case UCG_MSG_DRAW_L90TC:
			//ucg_handle_l90tc(ucg, ucg_dev_ic_st7735_18);
			ucg_handle_st7735_l90tcw(ucg);
			return 1;	
#endif /* UCG_MSG_DRAW_L90TC */
#ifdef UCG_MSG_DRAW_L90BF
		case UCG_MSG_DRAW_L90BF:
			ucg_handle_l90bf(ucg, ucg_dev_ic_st7735_18);
			return 1;
#endif /* UCG_MSG_DRAW_L90BF */
      
			/* msg UCG_MSG_DRAW_L90SE is handled by ucg_dev_default_cb */
			/*
		case UCG_MSG_DRAW_L90SE:
			return ucg->ext_cb(ucg, msg, data);
		*/
	}
	return ucg_dev_default_cb(ucg, msg, data);  
}

ucg_int_t ucg_ext_st7735W_18(ucg_t *ucg, ucg_int_t msg, void *data)
{
  switch(msg)
  {
    case UCG_MSG_DRAW_L90SE:
      //ucg_handle_l90se(ucg, ucg_dev_ic_st7735_18);
      ucg_handle_st7735_l90sew(ucg);
      break;
  }
  return 1;
}