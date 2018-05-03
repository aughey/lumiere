#include <FastLED.h>

#define LED_PIN     4
#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define NUM_LEDS    12

#define BRIGHTNESS  200
#define FRAMES_PER_SECOND 30

bool gReverseDirection = false;

CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];

// Fire2012 with programmable Color Palette
//
// This code is the same fire simulation as the original "Fire2012",
// but each heat cell's temperature is translated to color through a FastLED
// programmable color palette, instead of through the "HeatColor(...)" function.
//
// Four different static color palettes are provided here, plus one dynamic one.
//
// The three static ones are:
//   1. the FastLED built-in HeatColors_p -- this is the default, and it looks
//      pretty much exactly like the original Fire2012.
//
//  To use any of the other palettes below, just "uncomment" the corresponding code.
//
//   2. a gradient from black to red to yellow to white, which is
//      visually similar to the HeatColors_p, and helps to illustrate
//      what the 'heat colors' palette is actually doing,
//   3. a similar gradient, but in blue colors rather than red ones,
//      i.e. from black to blue to aqua to white, which results in
//      an "icy blue" fire effect,
//   4. a simplified three-step gradient, from black to red to white, just to show
//      that these gradients need not have four components; two or
//      three are possible, too, even if they don't look quite as nice for fire.
//
// The dynamic palette shows how you can change the basic 'hue' of the
// color palette every time through the loop, producing "rainbow fire".

CRGBPalette16 gPal;
#define NUM_PALLETS 4
CRGBPalette16 pallets[] = {
  CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Orange, CRGB::White),
  CRGBPalette16( CRGB::Black, CRGB::Yellow, CRGB::Orange, CRGB::White),
  CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White),
  CRGBPalette16( CRGB::Black, CRGB::Green, CRGB::Aqua,  CRGB::White)
};
int selected_pallet = false;

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
////
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation,
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking.
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 80

class FireSim {
  private:
    // Array of temperature readings at each simulation cell
#define HEAT_CELL_MULTIPLIER 2
#define HEAT_CELLS (NUM_LEDS*HEAT_CELL_MULTIPLIER)
    byte heat[HEAT_CELLS];
    bool fire_on;
  
  public:
  FireSim() {
    for(int i=0;i<HEAT_CELLS;++i) {
      heat[i] = 0;
    }
    fire_on = true;
  }
    bool isOn() {
      return fire_on;
    }
    bool isOff() {
      return !isOn();
    }
    void off() {
      fire_on = false;
    }
    void on() {
      fire_on = true;
    }
    void update()
    {

      // Step 1.  Cool down every cell a little
      for ( int i = 0; i < HEAT_CELLS; i++) {
        heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / HEAT_CELLS) + 2));
      }

      // Step 2.  Heat from each cell drifts 'up' and diffuses a little
      for ( int k = HEAT_CELLS - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
      }

      if(fire_on != true) {
        return;
      }

      // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
      if ( random8() < SPARKING ) {
        int y = random8(3);
        heat[y] = qadd8( heat[y], random8(160, 255) );
      }
    }

    void copyToLEDs(CRGB *thisleds)
    {
      // Step 4.  Map from heat cells to LED colors
      for ( int j = 0; j < NUM_LEDS; j++) {
        CRGB sum = CRGB(0, 0, 0);

        for (int m = 0; m < HEAT_CELL_MULTIPLIER; ++m) {

          // Scale the heat value from 0-255 down to 0-240
          // for best results with color palettes.
          byte colorindex = scale8( heat[j * HEAT_CELL_MULTIPLIER + m], 240);
          CRGB color = ColorFromPalette( gPal, colorindex);
          sum += color;
        }
        int pixelnumber;
        if ( gReverseDirection ) {
          pixelnumber = (NUM_LEDS - 1) - j;
        } else {
          pixelnumber = j;
        }
        //continue;
        // Serial.println(pixelnumber);
        thisleds[pixelnumber] = CRGB(
                                  sum[0] / (float)(HEAT_CELL_MULTIPLIER),
                                  sum[1] / (float)(HEAT_CELL_MULTIPLIER),
                                  sum[2] / (float)(HEAT_CELL_MULTIPLIER));

      }
    }
};

class Button {
  public:
    Button(int pin, int frames_on, int frames_off) {
      m_pin = pin;
      m_frames_on = frames_on;
      m_frames_off = frames_off;
      m_frames = -1;
      m_down = false;
    }
    void init() {
      pinMode(m_pin, INPUT_PULLUP);
    }
    bool clicked() {
      if(m_down == false && m_frames == m_frames_on) {
        return true;
      } else {
        return false;
      }
    }
    bool isPressed() {
      return m_down;
    }
    void tick() {
      if (digitalRead(m_pin) == 0) {
        // Down
        if (m_down) {
          m_frames = -1;
          return true;
        } else if (m_frames < 0) {
          // First time pressing
          m_frames = m_frames_on;
          return false;
        } else if (m_frames > 0) {
          m_frames--;
          return false;
        } else {
          // m_frames == 0
          m_down = true;
          m_frames = -1;
          return true;
        }
      } else {
        // Same deal as down
        if (m_down == false) {
          m_frames = -1;
          return false;
        } else if (m_frames < 0) {
          // First time releasing
          m_frames = m_frames_off;
          return true;
        } else if (m_frames > 0) {
          m_frames--;
          return true;
        } else {
          // m_frames == 0
          m_down = false;
          m_frames = -1;
          return true;
        }
      }
    }
  private:
    bool m_down;
    int m_pin;
    int m_frames;
    int m_frames_on;
    int m_frames_off;
};

FireSim sim1;
FireSim sim2;
Button top_button(6, 2, 2);
Button side_top_button(7, FRAMES_PER_SECOND*2, 2);
Button side_bottom_button(8, FRAMES_PER_SECOND*2, 2);

unsigned long nextwake = 0;

void setup() {
  delay(100); // sanity delay
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds1, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.addLeds < CHIPSET, LED_PIN + 1, COLOR_ORDER > (leds2, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );

  // This first palette is the basic 'black body radiation' colors,
  // which run from black to red to bright yellow to white.
  //gPal = HeatColors_p;
  gPal = pallets[0];

  // These are other ways to set up the color palette for the 'fire'.
  // First, a gradient from black to red to yellow to white -- similar to HeatColors_p
  //   gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);

  // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
  //gPal = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);

  // Third, here's a simpler, three-step gradient, from black to red to white
  //   gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::White);
  FastLED.show();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  top_button.init();
  side_top_button.init();
  side_bottom_button.init();

  Serial.begin(9600);

  nextwake = millis() + 1000 / FRAMES_PER_SECOND;

}


int brightness_direction = 0;
void loop()
{

  while (true) {
    // Add entropy to random number generator; we use a lot of it.
    random16_add_entropy( random());

    top_button.tick();
    side_top_button.tick();
    side_bottom_button.tick();

    if (side_top_button.clicked()) {
      sim1.on();
      sim2.on();
    }
    if (side_bottom_button.clicked()) {
      //FastLED.setBrightness(0);
      sim1.off();
      sim2.off();
    }

    if(side_top_button.isPressed()) {
      brightness_direction = 1;
      sim1.on();
      sim2.on();
    } else if(side_bottom_button.isPressed()) {
      if(FastLED.getBrightness() != 0) {
        brightness_direction = -1;
        sim1.on();
        sim2.on();
      }
    } else {
      brightness_direction = 0;
    }
    if (brightness_direction != 0) {
      int brightness = FastLED.getBrightness();
      brightness += brightness_direction;
#define MAX_BRIGHTNESS 255
      if (brightness >= MAX_BRIGHTNESS) {
        brightness = MAX_BRIGHTNESS;
        brightness_direction = 0;
      } else if (brightness <= 0) {
        brightness = 0;
        brightness_direction = 0;
      }
      Serial.print("Brightness = ");
      Serial.println(brightness);
      FastLED.setBrightness(brightness);
    }

    if (top_button.clicked()) {
      selected_pallet = (selected_pallet + 1) % NUM_PALLETS;
      gPal = pallets[selected_pallet];
    }


    //delay(100);
    //continue;


    // Fourth, the most sophisticated: this one sets up a new palette every
    // time through the loop, based on a hue that changes every time.
    // The palette is a gradient from black, to a dark color based on the hue,
    // to a light color based on the hue, to white.
    //
    //   static uint8_t hue = 0;
    //   hue++;
    //   CRGB darkcolor  = CHSV(hue,255,192); // pure hue, three-quarters brightness
    //   CRGB lightcolor = CHSV(hue,128,255); // half 'whitened', full brightness
    //   gPal = CRGBPalette16( CRGB::Black, darkcolor, lightcolor, CRGB::White);

    sim1.update();
    sim2.update();
    sim1.copyToLEDs(leds1);
    sim2.copyToLEDs(leds2);
    //Fire2012WithPalette(); // run simulation frame, using palette colors

    FastLED.show(); // display this frame

    // Sleep until the next wake
    long tosleep = nextwake - millis();
    FastLED.delay(max(nextwake - millis(), 0));
    nextwake = nextwake + 1000 / FRAMES_PER_SECOND;
  }
}



