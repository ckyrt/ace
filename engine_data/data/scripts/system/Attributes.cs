using System;
using System.Runtime.CompilerServices;

namespace Ace
{
namespace Core
{

[AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
public sealed class RangeAttribute : Attribute
{
    public readonly float min;

    public readonly float max;

    //
    // Summary:
    //     Attribute used to make a float or int variable in a script be restricted to a
    //     specific range.
    //
    // Parameters:
    //   min:
    //     The minimum allowed value.
    //
    //   max:
    //     The maximum allowed value.
    public RangeAttribute(float min, float max)
    {
        this.min = min;
        this.max = max;
    }
}

[AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
public sealed class MaxAttribute : Attribute
{
    public readonly float max;

    //
    // Summary:
    //     Attribute used to make a float or int variable in a script be restricted to a
    //     specific range.
    //
    //   max:
    //     The maximum allowed value.
    public MaxAttribute(float max)
    {
        this.max = max;
    }
}

[AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
public sealed class MinAttribute : Attribute
{
    public readonly float min;


    //
    // Summary:
    //     Attribute used to make a float or int variable in a script be restricted to a
    //     specific range.
    //
    // Parameters:
    //   min:
    //     The minimum allowed value.
   
    public MinAttribute(float min)
    {
        this.min = min;
    }
}

[AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
public sealed class StepAttribute : Attribute
{
    public readonly float step;

   
    public StepAttribute(float step)
    {
        this.step = step;
    }
}


[AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
public sealed class TooltipAttribute : Attribute
{
    public readonly string tooltip;

    public TooltipAttribute(string tooltip)
    {
        this.tooltip = tooltip;
    }
}

}

}

