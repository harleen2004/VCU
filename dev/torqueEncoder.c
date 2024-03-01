#include <stdlib.h>  //Needed for malloc
#include <math.h>
#include "IO_RTC.h"
#include "torqueEncoder.h"
#include "mathFunctions.h"
#include "sensors.h"


/*****************************************************************************
* Torque Encoder (TPS) functions
* RULE EV2.3.5:
* If an implausibility occurs between the values of these two sensors the power to the motor(s) must be immediately shut down completely.
* It is not necessary to completely deactivate the tractive system, the motor controller(s) shutting down the power to the motor(s) is sufficient.
****************************************************************************/
TorqueEncoder* TorqueEncoder_new()
{
    TorqueEncoder* me = (TorqueEncoder*)malloc(sizeof(struct _TorqueEncoder));
    me->tps0 = &Sensor_TPS0;
    me->tps1 = &Sensor_TPS1;

    //Where/should these be hardcoded?
    me->tps0_reverse = FALSE;
    me->tps1_reverse = TRUE;

    me->outputCurveExponent = 1.0;

    me->travelPercent = 0;
    me->runCalibration = FALSE;  //Do not run the calibration at the next main loop cycle

    //me->calibrated = FALSE;
    //TorqueEncoder_resetCalibration(me);

    //Old Rotary Sensor Datasheet limits (used by safety checker / rules requirement)
    //Datasheet had an operating range
    //me->tps0->specMin = 5000 * .05 - 5000 * .006; // = 220 , Target 0% = ~500
    //me->tps0->specMax = 5000 * .45 + 5000 * .006; // = 2280, Target 100% = ~2000
    //me->tps1->specMin = 5000 * .55 - 5000 * .006; // = 2720, Target 0% = ~3000
    //me->tps1->specMax = 5000 * .95 + 5000 * .006; // = 4780, Target 100% = ~4600

    //New Rotary Sensor Datasheet limits (used by safety checker / rules requirement)
    //It is literally a potentiometer, no sensor operating range in theory?
    //That would mean we could probably make our own ranges up
    me->tps0->specMin = 100; // Target 0% = ~250
    me->tps0->specMax = 2500; // Target 100% = ~2000
    me->tps1->specMin = 2500; // Target 0% = ~2650
    me->tps1->specMax = 4900; // Target 100% = ~4700

    
    me->tps0_calibMin = 500;
    me->tps0_calibMax = 1440;
    me->tps1_calibMin = 3350;
    me->tps1_calibMax = 4400;
    me->calibrated = TRUE;

    return me;
}

//Updates all values based on sensor readings, safety checks, etc
void TorqueEncoder_update(TorqueEncoder* me)
{
    me->tps0_value = me->tps0->sensorValue;
    me->tps1_value = me->tps1->sensorValue;

    me->travelPercent = 0;
    ubyte2 errorCount = 0;
    
    //This function runs before the calibration cycle function.  If calibration is currently
    //running, then set the percentage to zero for safety purposes.
    if (me->runCalibration == TRUE)
    {
        errorCount++;  //DO SOMETHING WITH THIS
    }
    else
    {
        //getPedalTravel = 0;

        //-------------------------------------------------------------------
        // Make sure the sensors have been calibrated
        //-------------------------------------------------------------------
        //if ((Sensor_TPS0.isCalibrated == FALSE) || (Sensor_TPS1.isCalibrated == FALSE))
        if (me->calibrated == FALSE)
        {
            me->tps0_percent = 0;
            me->tps1_percent = 0;
            (errorCount)++;  //DO SOMETHING WITH THIS
        }
        else
        {
            //Calculate individual throttle percentages
            //Percent = (Voltage - CalibMin) / (CalibMax - CalibMin)
            me->tps0_percent = getPercent(me->tps0_value, me->tps0_calibMin, me->tps0_calibMax, TRUE);
            me->tps1_percent = getPercent(me->tps1_value, me->tps1_calibMin, me->tps1_calibMax, TRUE);

            me->travelPercent = (me->tps0_percent + me->tps1_percent) / 2;
        }
    }
}

void TorqueEncoder_resetCalibration(TorqueEncoder* me)
{
    me->calibrated = FALSE;
    
    me->tps0_calibMin = me->tps0->sensorValue;
    me->tps0_calibMax = me->tps0->sensorValue;
    me->tps1_calibMin = me->tps1->sensorValue;
    me->tps1_calibMax = me->tps1->sensorValue;
}

void TorqueEncoder_startCalibration(TorqueEncoder* me, ubyte1 secondsToRun)
{
    if (me->runCalibration == FALSE) //Ignore the button if calibration is already running
    {
        me->runCalibration = TRUE;
        TorqueEncoder_resetCalibration(me);
        me->calibrated = FALSE;
        IO_RTC_StartTime(&(me->timestamp_calibrationStart));
        me->calibrationRunTime = secondsToRun;
    }
    else
    {
        IO_RTC_StartTime(&(me->timestamp_calibrationStart));  //extend the calibration time
    }
}

/*-------------------------------------------------------------------
* CalibrateTPS
* Description: Records TPS minimum/maximum voltages (when?) and stores them (where?), or flags that calibration is complete
* Parameters:
* Inputs:
* Returns:
* Notes:
* Throws:
-------------------------------------------------------------------*/
// Physical pedal travel will only occur across the center (about 1/2) of the actual sensor's range of travel
// The rules (especially EV2.3.6) are written about % of PEDAL travel, not percent of sensor range, so we must calculate pedal travel by recording the min/max voltages at min/max throttle positions
void TorqueEncoder_calibrationCycle(TorqueEncoder* me, ubyte1* errorCount)
//THIS FUNCTION SHOULD NOT BE CALLED FROM MAIN
{
    if (me->runCalibration == TRUE)
    {
        if (IO_RTC_GetTimeUS(me->timestamp_calibrationStart) < (ubyte4)(me->calibrationRunTime) * 1000 * 1000)
        {
            //The calibration itself
            if (me->tps0->sensorValue < me->tps0_calibMin) { me->tps0_calibMin = me->tps0->sensorValue; }
            if (me->tps0->sensorValue > me->tps0_calibMax) { me->tps0_calibMax = me->tps0->sensorValue; }

            if (me->tps1->sensorValue < me->tps1_calibMin) { me->tps1_calibMin = me->tps1->sensorValue; }
            if (me->tps1->sensorValue > me->tps1_calibMax) { me->tps1_calibMax = me->tps1->sensorValue; }

        }
        else  //Calibration shutdown
        {
            float4 shrink0 = (me->tps0_calibMax - me->tps0_calibMin) * .05;
            float4 shrink1 = (me->tps1_calibMax - me->tps1_calibMin) * .05;
            me->tps0_calibMin += shrink0;
            me->tps0_calibMax -= shrink0;
            me->tps1_calibMin += shrink1;
            me->tps1_calibMax -= shrink1;

            me->runCalibration = FALSE;
            me->calibrated = TRUE;
            Light_set(Light_dashEco, 0);

        }

    }

}


void TorqueEncoder_getIndividualSensorPercent(TorqueEncoder* me, ubyte1 sensorNumber, float4* percent)
{
    switch (sensorNumber)
    {
    case 0:
        *percent = me->tps0_percent;
        break;
    case 1:
        *percent = me->tps1_percent;
        break;
    }
}


/*-------------------------------------------------------------------
* GetThrottlePosition
* Description: Reads TPS Pin voltages and returns % of throttle pedal travel.
* Parameters:  None
* Inputs:      Assumes TPS#.sensorValue has been set by main loop
* Returns:     Throttle value in percent (from 0 to 1)
* Notes:       Valid pedal travel is from 10% (0.10) to 90% (0.90), not including mechanical limits.
* Throws:      000 - TPS0 voltage out of range
*              001 - TPS1 voltage out of range, 002
-------------------------------------------------------------------*/
void TorqueEncoder_getPedalTravel(TorqueEncoder* me, ubyte1* errorCount, float4* pedalPercent)
{
    *pedalPercent = me->travelPercent;

    //What about other error states?
    //Voltage outside of calibration range
    //Voltages off center

    //    if (errorCount > 0)
    //    {
    //        return 0;
    //    }
    //    else
    //    {
    //return (TPS0PedalPercent + TPS1PedalPercent) / 2;
    //    }
}

void TorqueEncoder_getOutputPercent(TorqueEncoder* me, float4* outputPercent)
{
    *outputPercent = powf(me->travelPercent, me->outputCurveExponent);
}
