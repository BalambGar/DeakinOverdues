#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h> //sleep()
#include <stdlib.h> //malloc()
#include <bcm2835.h>
#include <curl/curl.h>
#include "nfc.h"

#define pin_1 RPI_GPIO_P1_1   // 3.3V
#define pin_2 RPI_GPIO_P1_2   // 5V
#define pin_3 RPI_GPIO_P1_3   // 2
#define pin_4 RPI_GPIO_P1_4   // 5V
#define pin_5 RPI_GPIO_P1_5   // 3
#define pin_6 RPI_GPIO_P1_6   // GND
#define pin_7 RPI_GPIO_P1_7   // 4
#define pin_8 RPI_GPIO_P1_8   // 14 (TXD)
#define pin_9 RPI_GPIO_P1_9   // GND
#define pin_10 RPI_GPIO_P1_10 // 15 (RXD)
#define pin_11 RPI_GPIO_P1_11 // 17
#define pin_12 RPI_GPIO_P1_12 // 18
#define pin_13 RPI_GPIO_P1_13 // 27
#define pin_14 RPI_GPIO_P1_14 // GND
#define pin_15 RPI_GPIO_P1_15 // 22
#define pin_16 RPI_GPIO_P1_16 // 23
#define pin_17 RPI_GPIO_P1_17 // 3.3V
#define pin_18 RPI_GPIO_P1_18 // 24
#define pin_19 RPI_GPIO_P1_19 // 10
#define pin_20 RPI_GPIO_P1_20 // GND
#define pin_21 RPI_GPIO_P1_21 // 9
#define pin_22 RPI_GPIO_P1_22 // 25
#define pin_23 RPI_GPIO_P1_23 // 11
#define pin_24 RPI_GPIO_P1_24 // 8
#define pin_25 RPI_GPIO_P1_25 // GND
#define pin_26 RPI_GPIO_P1_26 // 7

int trig_pin;
int echo_pin;
uint32_t nfc_curr_id = 0;

float get_reading(float max_distance)
{
    if (!bcm2835_init())
    {
        printf("Error: failed bcm2835_init(). You may need root privledges as this function requires access"
           "to /dev/mem.");
        return 1;
    }

    bcm2835_gpio_fsel(trig_pin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(echo_pin, BCM2835_GPIO_FSEL_INPT);

    // Send initial trigger signal, requires HIGH for at least 10 microseconds (us)
    bcm2835_gpio_write(trig_pin, HIGH);
    bcm2835_delayMicroseconds(10);
    bcm2835_gpio_write(trig_pin, LOW);

    struct timeval tv;
    double start_time_us, signal_off;
    gettimeofday(&tv, NULL);
    start_time_us = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0f;
    do {
       gettimeofday(&tv, NULL);
       signal_off = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0f;
       if ((signal_off - start_time_us) * 17180.0 > max_distance) // Time out if max distance is surpassed
       {
           bcm2835_close();
           return -1.0;
       }
   } while (bcm2835_gpio_lev(echo_pin) == 0);

   double signal_on;
   do 
   {
       gettimeofday(&tv, NULL);
       signal_on = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0f;
       if ((signal_on - signal_off) * 17180.0 > max_distance) // Time out if max distance is surpassed
       {
           bcm2835_close();
           return -1.0;
       }
   } while (bcm2835_gpio_lev(echo_pin) == 1);

   double time_passed = signal_on - signal_off;
   double distance = time_passed * 17180.0;

   bcm2835_close();
   return distance;
}

int bcm_micro_sleep(uint64_t microsecond_delay) //For use when bcm2835_init() hasn't been called
{
    if (!bcm2835_init())
        return 1;
    bcm2835_delayMicroseconds(microsecond_delay);
    bcm2835_close();
    return 0;
}


const char* values[8];
const char** get_arguments(int argc, char** argv)
{
    //values[] format: [trigger, echo, time, distance, delay, verbose   , expected] //TODO: Remove time, is no longer used
    //Converts to    : [int    , int , int , float   , int  , int (bool), float]
    for (int j = 0; j < 8; j++)
        values[j] = "";

    int i = 1;
    for (; i < argc; i++)
    {
        char* curr = argv[i];
        if (strcmp(curr, "-g") == 0){
            values[0] = argv[++i];
        }
        else if (strcmp(curr, "-c") == 0){
            values[1] = argv[++i];
        }
        else if (strcmp(curr, "-t") == 0){
            values[2] = argv[++i];
        }
        else if (strcmp(curr, "-d") == 0){
            values[3] = argv[++i];
        }
        else if (strcmp(curr, "-e") == 0){
            values[4] = argv[++i];
        }
        else if (strcmp(curr, "-v") == 0){
            values[5] = "1";
        }
        else if (strcmp(curr, "-x") == 0){
            values[6] = argv[++i];
        }
        else
            printf("Illegal argument found: '%s'\n", curr);
    }

    return values;
}

//Returns: 0 if failed, 1 if sucessful
int nfc_initial_config(int verbose)
{
    uint32_t versiondata;
    uint32_t id;

    if (verbose)
        printf("Hello!\n");
    begin();
    versiondata = getFirmwareVersion();
    if (!versiondata)
    {
        if (verbose)
            printf("Didn't find PN53x board\n");
        return 0;
    }
    if (verbose)
    {
        // Got ok data, print it out!
        printf("Found chip PN5");
        printf("%x\n",((versiondata>>24) & 0xFF));
        printf("Firmware ver. ");
        printf("%d",((versiondata>>16) & 0xFF));
        printf(".");
        printf("%d\n",((versiondata>>8) & 0xFF));
        printf("Supports ");
        printf("%x\n",(versiondata & 0xFF));
    }
    SAMConfig();
    return 1;
}

//Call nfc_initial_config() before calling this!
uint32_t nfc_get_reading(int verbose)
{
    return readPassiveTargetID(PN532_MIFARE_ISO14443A);
}

//Loops until the sensor detects an object changing direction of motion back up
//min_rep_distance = Minimum distance that a rep should cover to be valid. Measured in cm.
//timeout_distance = The maximum distance that the ultrasonic sensor should wait for. Set this as low as possible for maximum performance.
//                   The HC-SRO4 sensor has a natural limit of about 400cm. Measured in cm. Setting too low will cause "false" timeouts.
//rep_timeout_sec = The maximum amount of time this method should wait for a rep to start/finish. If a rep takes longer than this time,
//                  -1 will be returned. Measured in seconds. Setting this too high may make the program seem unresponsive at times.
//verbose = 1 or 0. Will display extra information useful for debugging
//Returns: Distance that the rep covered. -1 if timed out, -2 if a new nfc is scanned
float wait_for_rep(float min_rep_distance, float timeout_distance, int rep_timeout_sec, int verbose)
{
    if (verbose)
        printf("Starting wait_for_rep()\n");
    float initial_distance = get_reading(timeout_distance);
    float min_value = initial_distance; //Track the minimum/maximum value so we know what value to return
    float max_value = initial_distance;
    const int MEASUREMENT_DELAY = 100 * 1000;// Delay between each sensor update, in us (micro seconds). Note that the delay
                                             // between two measurements will be: MEASUREMENT_DELAY + the time it takes to
                                             // take a measurement (depends on the distance reported, or on timeout_distance)

    //Variables needed for timeout functionality
    struct timeval tv;
    time_t start_time_s, curr_time_s;
    gettimeofday(&tv, NULL);
    start_time_s = tv.tv_sec;
    

    int noise_tolerance_counter = 0;
    const int COUNTER_LIMIT = 6; // * 100000 / MEASUREMENT_DELAY //Edit only the first number to adjust the counter.
    //Keep on getting measurements while the weight is going up and hits the min_rep_distance (measurements will decrease)
    if (verbose)
        printf("Going up\n");
    while (1)
    {
        //Check for timeout
        gettimeofday(&tv, NULL);
        curr_time_s = tv.tv_sec;
        int time_passed = curr_time_s - start_time_s;
        if (time_passed >= rep_timeout_sec)
            return -1f;

        //Check if a new NFC has been scanned
        uint32_t nfc_new_id = nfc_get_reading(verbose);
        if (nfc_new_id != 0 && nfc_new_id != nfc_curr_id)
        {
            nfc_curr_id = nfc_new_id;
            return -2f;
        }

        //Now continue with obtaining measurements
        float curr_reading = get_reading(timeout_distance);
        if (curr_reading == -1.0) //Do nothing if a timeout is reported
            continue;

        max_value = curr_reading > max_value ? curr_reading : max_value;
        min_value = curr_reading < min_value ? curr_reading : min_value;

        if (verbose)
            printf("max - curr = %f\n\tmax = %f\n\tcurr_reading = %f\n\tmin = %f\n\tnoise_tolerance_counter = %d\n",
                max_value - curr_reading, max_value, curr_reading, min_value, noise_tolerance_counter);
        if (max_value - curr_reading > min_rep_distance)
        {
            if (noise_tolerance_counter++ >= COUNTER_LIMIT)
                break;
        } else
            noise_tolerance_counter = 0; //Reset counter if weight is not above minimum threshold long enough.
                                         //Be careful with the value of COUNTER_LIMIT or a reasonably paced rep will not get picked up.

            bcm_micro_sleep(MEASUREMENT_DELAY);
    }

    if (verbose)
        printf("\nAnd down\n");
    noise_tolerance_counter = 0;
    //Now get measurements until the weight is below the min_rep_distance
    while (1)
    {
        //Check for timeout
        gettimeofday(&tv, NULL);
        curr_time_s = tv.tv_sec;
        int time_passed = curr_time_s - start_time_s;
        if (time_passed >= rep_timeout_sec)
            return -1f;

        //Check if a new NFC has been scanned
        uint32_t nfc_new_id = nfc_get_reading(verbose);
        if (nfc_new_id != 0 && nfc_new_id != nfc_curr_id)
        {
            nfc_curr_id = nfc_new_id;
            return -2f;
        }

        //Now continue with obtaining measurements
        float curr_reading = get_reading(timeout_distance);
        if (curr_reading == -1.0) //Do nothing if a timeout is reported
            continue;

        min_value = curr_reading < min_value ? curr_reading : min_value;

        if (verbose)
            printf("max - curr = %f\n\tmax = %f\n\tcurr_reading = %f\n\tmin = %f\n\tnoise_tolerance_counter = %d\n",
                max_value - curr_reading, max_value, curr_reading, min_value, noise_tolerance_counter);
        if (curr_reading - min_value < min_rep_distance)
        {
            if (noise_tolerance_counter++ >= COUNTER_LIMIT)
                break;
        } else
        noise_tolerance_counter = 0;

        bcm_micro_sleep(MEASUREMENT_DELAY);
    }

    //Finally, return the distance that the weight was pulled during the rep
    return max_value - min_value;
}

void nfc_read_blocking(int verbose)
{
    if (verbose)
        printf("Waiting for an NFC to get scanned...\n");
    nfc_curr_id = 0;
    do
    {
        nfc_curr_id = nfc_get_reading(verbose);
    } while (nfc_curr_id != 0);
    if (verbose)
        printf("NFC Scanned succesfully! ID = %d\n", nfc_curr_id);
}

void send_data(uint32_t id, int total_reps, float average_distance)
{
    CURLcode res;
    CURL *curl;

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive thedata. */
        curl_easy_setopt(curl, CURLOPT_URL, "http://203.42.134.77/api/rfid");
        /* Now specify the POST data */
        char str[100];
        sprintf(str, "userID=12453&rfid=%d",id);
        printf(str);                                    
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        printf("usage: ./usonic -g trigger_pin -c echo_pin -x expected [-r rate] [-d distance] "
           "[-e delay] [-v]\n");
        return 1;
    }
    const char** args = get_arguments(argc, argv);

    // *** START - Retrieve data from args ***
    //GPIO pin numbers
    if (strcmp(args[0], "") == 0) //Trigger
    {
        printf("Error: no trigger pin number provided (-g).\n");
        return 1;
    }
    trig_pin = atol(args[0]);
    if (strcmp(args[1], "") == 0) //Echo
    {
        printf("Error: no echo pin number provided (-c).\n");
        return 1;
    }
    echo_pin = atol(args[1]);
    //Get the minimum distance that the weight must be pulled
    if (strcmp(args[6], "") == 0)
    {
        printf("Error: No expected distance for weight specified");
        return 1;
    }
    float expected_distance = atof(args[6]);
    //Max distance (cm) before timeout
    float max_distance = 200.0f;
    if (strcmp(args[3], "") != 0)
        max_distance = atof(args[3]);
    //Verbosity (bool)
    uint8_t verbose = 0;
    if (strcmp(args[5], "") != 0)
        verbose = 1;
    //Additional delay before execution, for user convenience
    if (strcmp(args[4], "") != 0)
    {
        if (verbose)
        {
            printf("Delaying for %d seconds...", atoi(args[4]));
            fflush(stdout);
        }
        sleep(atoi(args[4]));
        if (verbose)
            printf(" Finished delay.\n");
    }
    // *** END - Retrieve data from args ***

    //Variables for time management
    struct timeval tv;
    time_t start_time_s, curr_time_s;
    gettimeofday(&tv, NULL);
    start_time_s = tv.tv_sec;

    if (nfc_initial_config(verbose) == 0)
    {
        printf("Error: Could not initialise NFC reader. Exiting.\n");
        return 1;
    }
    //Wait for an NFC scan before starting the main loop
    nfc_read_blocking(verbose);
    uint32_t nfc_reporting_id = nfc_curr_id; //Keep track of the working id so we can send it when we're done

    //Variables for tracking
    int total_reps = 0;
    float total_distance = 0f;
    while (1)
    {
        if (verbose)
            printf("Starting main loop\n");

        // Start a new sensor reading
        float rep_distance = wait_for_rep(expected_distance, max_distance, 10, verbose); //Recommended safe default values: 20, 200, 10
        if (rep_distance == -1f)
        {
            if (verbose)
                printf("Workout session timed out, waiting for a new user");
            total_reps = 0;
            total_distance = 0f;
            if (total_reps != 0)
                send_data(nfc_reporting_id, total_reps, total_distance / total_reps);
            nfc_read_blocking(verbose);
            nfc_reporting_id = nfc_curr_id;
        } else if (rep_distance == -2f)
        {
            if (verbose)
                printf("New ID scanned, sending data and setting up for new user");
            total_reps = 0;
            total_distance = 0f;
            if (total_reps != 0)
                send_data(nfc_reporting_id, total_reps, total_distance / total_reps);
            nfc_reporting_id = nfc_curr_id;
        } else //Valid rep, record it
        {
            total_distance += rep_distance;
            total_reps++;
            if (verbose)
                printf("*** Rep recorded: %1.2fcm\n", rep_distance);
        }
    }
    
    return 0;
}