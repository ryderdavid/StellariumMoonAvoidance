# Comparison: Stellarium Moon Avoidance vs NINA Target Scheduler

## Summary
Our implementation is **mostly compatible** with NINA's logic, but there are some differences in edge case handling.

## Relaxation Logic

### NINA Implementation (from MoonAvoidanceExpert.cs):
```csharp
// Apply relaxation ONLY when moonAltitude <= MoonRelaxMaxAltitude AND MoonRelaxScale > 0
if (moonAltitude <= exposure.MoonRelaxMaxAltitude && exposure.MoonRelaxScale > 0) {
    moonSeparationParameter = moonSeparationParameter + (exposure.MoonRelaxScale * (moonAltitude - exposure.MoonRelaxMaxAltitude));
    moonWidthParameter = moonWidthParameter * ((moonAltitude - exposure.MoonRelaxMinAltitude) / (exposure.MoonRelaxMaxAltitude - exposure.MoonRelaxMinAltitude));
}
```

### Our Implementation:
```cpp
// Apply relaxation when moonAltitude is within [MinAlt, MaxAlt]
if (moonAltitude >= filter.minAlt && moonAltitude <= filter.maxAlt) {
    separation = filter.separation + filter.relaxation * (moonAltitude - filter.maxAlt);
    width = filter.width * ((moonAltitude - filter.minAlt) / (filter.maxAlt - filter.minAlt));
}
```

**Difference**: 
- NINA checks `altitude <= maxAlt` (no lower bound check in the condition)
- We check `altitude >= minAlt && altitude <= maxAlt` (both bounds checked)

**Impact**: 
- When `altitude < minAlt`, NINA would still apply relaxation if `altitude <= maxAlt` (which is always true if `minAlt < maxAlt`), but we don't.
- However, NINA has a special case (see below) that turns avoidance OFF when `altitude <= minAlt`.

## Special Cases

### NINA Special Cases:

1. **Below Min Altitude** (lines 67-70):
```csharp
// Avoidance is completely off if the moon is below the relaxation min altitude and relaxation applies
if (moonAltitude <= exposure.MoonRelaxMinAltitude && exposure.MoonRelaxScale > 0) {
    return false; // Avoidance OFF
}
```

2. **Above Max Altitude with "Moon Must Be Down"** (lines 72-75):
```csharp
// Avoidance is absolute regardless of moon phase or separation if Moon Must Be Down is enabled
if (moonAltitude >= exposure.MoonRelaxMaxAltitude && exposure.MoonDownEnabled) {
    return true; // Always rejected (absolute avoidance)
}
```

3. **Negative Relaxed Separation** (lines 77-80):
```csharp
// If the separation was relaxed into oblivion, avoidance is off
if (moonSeparationParameter <= 0) {
    return false; // Avoidance OFF
}
```

### Our Implementation:
- **Below Min Altitude**: We use base separation (traditional avoidance), not OFF
- **Above Max Altitude**: We use base separation (traditional avoidance), no "Moon Must Be Down" feature
- **Negative Relaxed Separation**: We don't check for this

## Lorentzian Formula

### NINA:
Uses `AstrometryUtils.GetMoonAvoidanceLorentzianSeparation(moonAge, moonSeparationParameter, moonWidthParameter)`
- The exact implementation is not shown in the provided code, but it likely uses a similar Lorentzian formula

### Our Implementation:
```cpp
// SEPARATION = DISTANCE / ( 1 + (AGE / WIDTH)^2 )
// where AGE is days from full moon (0 = full moon, 14.765 = new moon)
double normalizedTerm = moonAgeDays / adjustedWidth;
double lorentzianFactor = 1.0 + (normalizedTerm * normalizedTerm);
double radiusDegrees = adjustedSeparation / lorentzianFactor;
```

**Note**: We use `AGE = days from full moon`, which gives highest separation at full moon (AGE = 0).

## Key Differences Summary

| Aspect | NINA | Our Implementation | Impact |
|--------|------|-------------------|--------|
| Relaxation condition | `altitude <= maxAlt && relaxScale > 0` | `altitude >= minAlt && altitude <= maxAlt` | Different behavior when `altitude < minAlt` |
| Below min altitude | Avoidance OFF | Base separation (traditional) | **Different behavior** |
| Above max altitude | Can be absolute (if "Moon Must Be Down") | Base separation (traditional) | **Different behavior** |
| Negative separation | Avoidance OFF | Not checked | **Different behavior** |
| Lorentzian formula | Uses `GetMoonAvoidanceLorentzianSeparation` | `DISTANCE / (1 + (AGE/WIDTH)^2)` | Should be equivalent if NINA uses same formula |

## Recommendations

To match NINA's behavior exactly, we should:

1. **Add check for below min altitude**: If `moonAltitude <= minAlt && relaxation > 0`, turn avoidance OFF (or don't draw circle)
2. **Add check for negative separation**: If relaxed `separation <= 0`, turn avoidance OFF
3. **Consider "Moon Must Be Down" feature**: Add option for absolute avoidance when moon is above max altitude
4. **Verify Lorentzian formula**: Confirm that NINA's `GetMoonAvoidanceLorentzianSeparation` uses the same formula we're using

## Current Status

✅ **Core relaxation formulas match**: The separation and width adjustment formulas are identical
✅ **Lorentzian formula**: We use the standard Lorentzian formula with days from full moon
⚠️ **Edge cases differ**: Below min altitude and negative separation handling differ
⚠️ **Missing feature**: "Moon Must Be Down" mode not implemented

