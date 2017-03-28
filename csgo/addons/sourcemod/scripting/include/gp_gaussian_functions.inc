/**
 * GoonPUG Trueskill Gaussian Functions module
 */
 
public Float:GaussianVExceedsMargin(Float:teamPerformanceDifference, Float:drawMargin)
{
    new Float:denominator = GaussianCumulativeTo(teamPerformanceDifference - drawMargin);
    
    if (denominator < 2.222758749e-162)
        return ((-1.0 * teamPerformanceDifference) + drawMargin);
    
    return GaussianAt(teamPerformanceDifference - drawMargin) / denominator;
}

public Float:GaussianWExceedsMargin(Float:teamPerformanceDifference, Float:drawMargin)
{
    new Float:denominator = GaussianCumulativeTo(teamPerformanceDifference - drawMargin);
    
    if (denominator < 2.222758749e-162) {
        if (teamPerformanceDifference < 0.0)
            return 1.0;
        else
            return 0.0;
    }

    new Float:vWin = GaussianVExceedsMargin(teamPerformanceDifference, drawMargin);
    return (vWin * (vWin + teamPerformanceDifference - drawMargin));
}

public Float:GaussianVWithinMargin(Float:teamPerformanceDifference, Float:drawMargin)
{
    new Float:teamPerformanceDifferenceAbsoluteValue = FloatAbs(teamPerformanceDifference);
    new Float:denominator = GaussianCumulativeTo(drawMargin - teamPerformanceDifferenceAbsoluteValue) - GaussianCumulativeTo((-1.0 * drawMargin) - teamPerformanceDifferenceAbsoluteValue);
    
    if (denominator < 2.222758749e-162) {
        if (teamPerformanceDifference < 0.0) {
            return ((-1.0 * teamPerformanceDifference) - drawMargin);
        }

        return ((-1.0 * teamPerformanceDifference) + drawMargin);
    }
    
    new Float:numerator = GaussianAt((-1.0 * drawMargin) - teamPerformanceDifferenceAbsoluteValue) - GaussianAt(drawMargin - teamPerformanceDifferenceAbsoluteValue);
    
    if (teamPerformanceDifference < 0.0) {
        return ((-1.0 * numerator) / denominator);
    }

    return (numerator / denominator);
}

public Float:GaussianWWithinMargin(Float:teamPerformanceDifference, Float:drawMargin)
{
    new Float:teamPerformanceDifferenceAbsoluteValue = FloatAbs(teamPerformanceDifference);
    new Float:denominator = GaussianCumulativeTo(drawMargin - teamPerformanceDifferenceAbsoluteValue) - GaussianCumulativeTo((-1.0 * drawMargin) - teamPerformanceDifferenceAbsoluteValue);
    
    if (denominator < 2.222758749e-162) {
        return 1.0;
    }

    new Float:vt = GaussianVWithinMargin(teamPerformanceDifferenceAbsoluteValue, drawMargin);

    return (vt * vt) + ((drawMargin - teamPerformanceDifferenceAbsoluteValue) * GaussianAt(drawMargin - teamPerformanceDifferenceAbsoluteValue) - ((-1.0 * drawMargin) - teamPerformanceDifferenceAbsoluteValue) * GaussianAt((-1.0 * drawMargin) - teamPerformanceDifferenceAbsoluteValue)) / denominator;
}

public Float:GaussianAt(Float:x)
{
    new Float:multiplier = 1.0 / (1.0 * Float:SquareRoot(2.0 * 3.141592653589793));
    new Float:expPart = Float:Exponential((-1.0 * (x * x)) / 2.0);
    
    new Float:result = multiplier * expPart;
    return result;
}

public Float:GaussianCumulativeTo(Float:x)
{
    new Float:invsqrt2 = -0.707106781186547524400844362104;
    new Float:result = GaussianErrorFunctionCumulativeTo(invsqrt2 * x);
    return (result / 2.0);
}

Float:GaussianErrorFunctionCumulativeTo(Float:x)
{          
    new Float:z = FloatAbs(x);
    
    new Float:t = 2.0 / (2.0 + z);
    new Float:ty = (4.0 * t) - 2.0;
    
    new Float:coefficients[] = {-1.3026537197817094, 6.4196979235649026e-1,
        1.9476473204185836e-2, -9.561514786808631e-3,
        -9.46595344482036e-4, 3.66839497852761e-4, 4.2523324806907e-5,
        -2.0278578112534e-5, -1.624290004647e-6, 1.303655835580e-6,
        1.5626441722e-8, -8.5238095915e-8, 6.529054439e-9,
        5.059343495e-9, -9.91364156e-10, -2.27365122e-10,
        9.6467911e-11, 2.394038e-12, -6.886027e-12, 8.94487e-13,
        3.13092e-13, -1.12708e-13, 3.81e-16, 7.106e-15, -1.523e-15,
        -9.4e-17, 1.21e-16, -2.8e-17};
    
    int cofLength = 28;
    new Float:d = 0.0;
    new Float:dd = 0.0;
    
    for (int j = cofLength - 1; j > 0; j--)
    {
		new Float:tmp = d;
		d = ((ty * d) - dd + coefficients[j]);
		dd = tmp;
	}
    
    new Float:ans = (t * Float:Exponential(((-1.0 * z) * z) + (0.5 * (coefficients[0] + (ty * d))) - dd));
    if (x >= 0.0)
    {
        return ans;
    }
    else
    {
        return (2.0 - ans);
    }
}

public Float:GaussianInverseCumulativeTo(Float:x)
{
    return (0.0 - (Float:SquareRoot(2.0) * GaussianInverseErrorFunctionCumulativeTo(2 * x)));
}

Float:GaussianInverseErrorFunctionCumulativeTo(Float:p)
{
    if (p >= 2.0)
        return -100.00;
    
    if (p <= 0.0)
        return 100.00;

    new Float:pp;
    if (p < 1.0)
    {
        pp = p;
    }
    else
    {
        pp = (2.0 - p);
    }
    
    new Float:t = Float:SquareRoot(-2.0 * Float:Logarithm(pp / 2.0, 10.00)); // Initial guess
    new Float:x = -0.70711 * ((2.30753 + (t * 0.27061)) / (1.0 + (t * (0.99229 + (t * 0.04481)))) - t);
    
    for (int j = 0; j < 2; j++)
    {
        new Float:err = GaussianErrorFunctionCumulativeTo(x) - pp;
        x += (err / (1.12837916709551257 * Float:Exponential(-1.0 * (x * x)) - (x * err))); // Halley
    }
    
    if (p < 1.0)
    {
        return x;
    }
    else
    {
        return (-1.0 * x);
    }
}