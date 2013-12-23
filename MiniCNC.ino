/* MiniCNC Arduino sketch by Oliv4945 
 * Use only absolute coordinates for now
 * Only X/Y axis
 * If needed feel free to contact me
 */


#include <Servo.h>

#define PIN_SERVO_X 4
#define PIN_SERVO_Y 3
#define LINE_BUFFER_LENGTH 512
#define MOVEMENT_MIN 1
#define MOVEMENT_MAX 179 
#define STEP_DELAY 7


/* Structures, global variables    */
struct point {
  float x;
  float y;
};

Servo servoX;
Servo servoY;
struct point actuatorPos;

/**********************
* void setup() - Initialisations
***********************/
void setup() {
  Serial.begin( 115200 );
  servoX.attach( PIN_SERVO_X );
  servoY.attach( PIN_SERVO_Y );

  // Set default positions
  servoX.write( 1 );
  servoY.write( 1 );
  actuatorPos.x = 1;
  actuatorPos.y = 1;
  
  Serial.println( "MiniCNC" );
  Serial.println( "OK" );
}

/**********************
* void loop() - Main loop
***********************/
void loop() {
  char line[ LINE_BUFFER_LENGTH ];
  char c;
  int lineIndex;
  bool lineIsComment, lineSemiColon;
  
  lineIndex = 0;
  lineSemiColon = false;
  lineIsComment = false;
  
  while (1) {
    
    // Serial reception - Mostly from Grbl, added semicolon support
    while ( Serial.available()>0 ) {
      c = Serial.read();
      if (( c == '\n') || (c == '\r') ) {             // End of line reached
        if ( lineIndex > 0 ) {                        // Line is complete. Then execute!
          line[ lineIndex ] = '\0';                   // Terminate string
          Serial.print( "Received : "); Serial.println( line );
          processIncomingLine( line, lineIndex );
          lineIndex = 0;
        } else { 
          // Empty or comment line. Skip block.
        }
        lineIsComment = false;
        lineSemiColon = false;  
      } else {
        if ( (lineIsComment) || (lineSemiColon) ) {   // Throw away all comment characters
          if ( c == ')' )  lineIsComment = false;     // End of comment. Resume line.
        } else {
          if ( c <= ' ' ) {                           // Throw away whitepace and control characters
          } else if ( c == '/' ) {                    // Block delete not supported. Ignore character.
          } else if ( c == '(' ) {                    // Enable comments flag and ignore all characters until ')' or EOL.
            lineIsComment = true;
          } else if ( c == ';' ) {
            lineSemiColon = true;
          } else if ( lineIndex >= LINE_BUFFER_LENGTH-1 ) {
            Serial.println( "ERROR - lineBuffer overflow" );
            lineIsComment = false;
            lineSemiColon = false;
          } else if ( c >= 'a' && c <= 'z' ) {        // Upcase lowercase
            line[ lineIndex++ ] = c-'a'+'A';
          } else {
            line[ lineIndex++ ] = c;
          }
        }
      }
    }
  }
}



/**********************
* void processIncomingLine( char* line, int charNB ) - Main loop
* char* line : Line to process
* int charNB : Number of characters
***********************/
void processIncomingLine( char* line, int charNB ) {
  int currentIndex = 0;
  char buffer[ 64 ];                                 // Hope that 64 is enough for 1 parameter
  struct point newPos;
  
  newPos.x = 0.0;
  newPos.y = 0.0;
  
  
  while( currentIndex < charNB ) {
    switch ( line[ currentIndex++ ] ) {              // Select command, if any
      case 'G':
        buffer[0] = line[ currentIndex++ ];          // /!\ Dirty - Only works with 2 digit commands
        buffer[1] = line[ currentIndex++ ];
        buffer[2] = '\0';
        switch ( atoi( buffer ) ){                   // Select G command
          case 00:                                   // G00 & G01 - Movement or fast movement. Same here
          case 01:
            // /!\ Dirty - Suppose that X is before Y
            char* indexX = strchr( line+currentIndex, 'X' );  // Get X/Y position in the string (if any)
            char* indexY = strchr( line+currentIndex, 'Y' );
            if ( indexY <= 0 ) {
              newPos.x = atof( indexX + 1); 
              newPos.y = actuatorPos.y;
            } else if ( indexX <= 0 ) {
              newPos.y = atof( indexY + 1);
              newPos.x = actuatorPos.x;
            } else {
              newPos.y = atof( indexY + 1);
              indexY = '\0';
              newPos.x = atof( indexX + 1);
            }
            drawLine( (int) actuatorPos.x,(int) actuatorPos.y, (int) newPos.x, (int) newPos.y );
            actuatorPos.x = newPos.x;
            actuatorPos.y = newPos.y;
            break;
        }
        break;
      case 'M':
        buffer[0] = line[ currentIndex++ ];        // /!\ Dirty - Only works with 3 digit commands
        buffer[1] = line[ currentIndex++ ];
        buffer[2] = line[ currentIndex++ ];
        buffer[3] = '\0';
        switch ( atoi( buffer ) ){
          case 114:                                // M114 - Repport position
            Serial.print( "Absolute position : X = " );
            Serial.print( actuatorPos.x );
            Serial.print( "  -  Y = " );
            Serial.println( actuatorPos.y );
            break;
          default:
            Serial.print( "Command not recognized : M");
            Serial.println( buffer );
        }
    }
  }
}




/*********************************
 * Draw a line from (x0;y0) to (x1;y1). Bresenham algorythm from http://rosettacode.org/wiki/Bitmap/Bresenham's_line_algorithm
* int (x1;y1) : Starting coordinates
* int (x2;y2) : Ending coordinates
**********************************/
void drawLine(int x0, int y0, int x1, int y1) {
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int err = (dx>dy ? dx : -dy)/2, e2;
  for(;;){
    servoX.write( x0 );
    servoY.write( y0 );
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
    // Serial.print( "X=" ); Serial.print( x0 ); Serial.print( "  -  Y=" ); Serial.println( y0 );
    delay( STEP_DELAY );           //delay for settling
  }
}


