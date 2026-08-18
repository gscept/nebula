//------------------------------------------------------------------------------
/**
    PQuatFeedbackLoop

    A specialized proportional feedback loop for rotations, using a
    quaternion representation.
*/
using Nebula;

class PQuatFeedbackLoop
{
    //--------------------------------------------------------------------------
    /** <summary>
        The time at which the state is currently in.

        You generally don't have to change this. If you need to feed delta time
        into the system, instead change the stepSize before calling Update.
        </summary>
    */
    public double time = 0.0; 
    public double stepSize = 0.001;
    public float gain = -1.0f;
    public Quaternion goal;
    public Quaternion state;
    public float lastError = 0.0f;
    
    //--------------------------------------------------------------------------
    /** <summary>
        Perform one step/loop/update of the PID controller
        </summary>
    */
    public Quaternion Update(double dt)
    {
        // compute angular error between state and goal
        float error = Quaternion.AngleBetween(this.state, this.goal);
        this.lastError = error;

        float lerp = (float)(error * this.gain * dt);
        this.state = Quaternion.Slerp(this.state, this.goal, lerp);
        this.state.Normalize();

        this.time += dt;

        return this.state;
    }

    //--------------------------------------------------------------------------
    /** <summary>
        Advance to curTime, and perform N steps based on stepSize.
        </summary>
    */
    public Quaternion AdvanceToTime(double curTime)
    {
        if (this.stepSize <= 0.0)
        {
            Debug.Log($"[PQuatFeedbackLoop] Invalid stepSize {this.stepSize}; skipping AdvanceToTime to avoid an infinite loop.\n");
            this.time = curTime;
            return this.state;
        }

        double dt = curTime - this.time;

        // catch time exceptions
        if (dt < 0.0)
        {
            this.time = curTime;
        }
        else if (dt > 0.5)
        {
            this.time = curTime;
        }

        while (this.time < curTime)
        {
            _ = this.Update(this.stepSize);
        }

        return this.state;
    }
};
