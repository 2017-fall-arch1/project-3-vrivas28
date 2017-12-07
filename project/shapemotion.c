/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"
#define GREEN_LED BIT6

u_char player01S = '0'; // player01 score counter
u_char player02S = '0'; // player02 score counter
int won = 0;
int v = 0;

AbRect paddle = {abRectGetBounds, abRectCheck, {4,14}}; /**< This is the dimensions for the players' paddle*/
AbRect line = {abRectGetBounds, abRectCheck, {0, 61}}; // vertical line that divides the players cou.
AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};
  
Layer ballLa = {		/**< Layer with the pong ball */
  (AbShape *)&circle8,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GOLD,
  0,
};
Layer fieldLayer = {		/* outline of the game field */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &ballLa
};
Layer lineLa = {		/**< Layer with a vertical line */
  (AbShape *)&line,
  {(screenWidth/2), (screenHeight/2)}, /**< Middle field divider */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &fieldLayer,
};

Layer player01La = {		/**< Layer with a Player01 paddle */
  (AbShape *)&paddle,
  {screenWidth/2-48, screenHeight/2+48}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &lineLa,
};
Layer player02La = {		/**< Layer with a Player02 paddle */
  (AbShape *)&paddle,
  {screenWidth/2+48, screenHeight/2-48}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &player01La,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = {&ballLa, {2,4}, 0}; //moving layer for pong ball
MovLayer ml1 = {&player01La, {0,3}, 0}; // moving layer for player01's paddle
MovLayer ml0 = {&player02La, {0,3}, 0}; // moving layer for player02's paddle

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence, MovLayer *ml0, MovLayer *ml3)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  int velocity;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
        velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
        newPos.axes[axis] += (2*velocity);
        }
        // checks if the ball touched the fence
        else if((shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0])){
            newPos.axes[0] = screenWidth/2;
            newPos.axes[1] = screenHeight/2;
            player01S = player01S - 255;// updates player01 score
           
        }
        // checks if the ball touched the fence
        else if((shapeBoundary.botRight.axes[0] > fence->botRight.axes[0])){
            newPos.axes[0] = screenWidth/2;
            newPos.axes[1] = screenHeight/2;
            player02S = player02S - 255; // updates player02 score
            
        }
        //checking if ball touched the player01's paddle
        else if(abShapeCheck(ml3->layer->abShape, &ml3->layer->posNext, &ml->layer->posNext)){//square
            int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
            newPos.axes[axis] += (2*velocity);
        }
        //checking if ball touched the player02's paddle
        else if(abShapeCheck(ml0->layer->abShape, &ml0->layer->posNext, &ml->layer->posNext)){//square
            int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
            newPos.axes[axis] += (2*velocity);
        }
        else{
            if(player01S == '5' || player02S == '5')
            {
                won = 1;
            }
        }
      /**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}
// THE FOLLOWING TWO METHODS WERE REFERENCED FROM LAURA GONZALEZ
void moveDown(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 1; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }	// this if statement handles when a collision happens in the fence
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}

void moveUp(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Sub(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 1; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }	// this if statement handles when a collision happens in the fence
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}

u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();
  buzzer_init();
  layerInit(&player02La);
  layerDraw(&player02La);
  layerGetBounds(&fieldLayer, &fieldFence);
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &player02La);
    movLayerDraw(&ml1, &player02La);
    movLayerDraw(&ml3, &player02La);
    
    u_int switches = p2sw_read();
    drawString5x7(3, 152, "Player1:", COLOR_RED, COLOR_BLACK);
    drawString5x7(72, 152, "Player2:", COLOR_GREEN, COLOR_BLACK);
    drawChar5x7(52,152, player01S, COLOR_RED, COLOR_BLACK);
    drawChar5x7(120,152, player02S, COLOR_GREEN, COLOR_BLACK);
    drawString5x7(50, 0, "PONG", COLOR_BLUE, COLOR_BLACK);
    
    if(!(switches & (1<<0))){
    	play(1);
    }
    if (!(switches & (1<<1))){
    	play(2);
    }
    if (!(switches & (1<<2))){
      play(1);
    }
    if (!(switches & (1<<3))){
      play(2);
    }
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  u_int switches = p2sw_read();
  if (count == 15) {
    if(won == 1)
    {
        if(player01S > player02S)
        {
            drawString5x7(20,60, "Player 1 Wins!!", COLOR_GREEN, COLOR_BLACK);}
        else
            drawString5x7(20,60, "Player 2 Wins!!", COLOR_GREEN, COLOR_BLACK);}
    else 
        mlAdvance(&ml3, &fieldFence, &ml1,&ml0);
  
    if(switches & (1<<0)){
      moveUp(&ml1, &fieldFence);
    }
    if(switches & (1<<1)){
      moveDown(&ml1, &fieldFence);
    }
    if(switches & (1<<2)){
      moveUp(&ml0, &fieldFence);
    }
    if(switches & (1<<3)){
      moveDown(&ml0, &fieldFence);
    }
  
      redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
