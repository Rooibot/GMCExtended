# GMCv2 Extended

This repository contains a simple plugin which extends GMCv2 in various ways, with utilities and such provided by GMC licensees.

(Mostly it exists because Packetdancer got tired of answering how to copy the trajectory movement component into Blueprint projects, and her solution was "just turn it into a plugin".)

There will be better documentation here eventually. Probably.

Things currently in the project:

## UGMCE_OrganicMovementCmp

This is an extension of GMCv2's Organic Movement Component, adding a few things:

* Stop/pivot prediction (for distance matching), available via 
* Trajectory prediction (for motion trajectory)
* Debug visualization options for the above two
* A very, _very_ basic ragdoll mode

**Trajectory:**  
[![Trajectory example video](http://img.youtube.com/vi/y0oFou7ww64/0.jpg)](https://www.youtube.com/watch?v=y0oFou7ww64)

**Ragdoll:**  
[![Ragdoll example video](http://img.youtube.com/vi/Vipoc2ISJp0/0.jpg)](https://www.youtube.com/watch?v=Vipoc2ISJp0)

Some details...

### Stop/Pivot Prediction

The component can be set to automatically keep stop/pivot predictions up-to-date via the `Precalculate Distance Matches` setting; they can also be manually calculated by calling `UpdateStopPredictions` and `UpdatePivotPredictions` instead. The calculated results are available via `IsStopPredicted` and `IsPivotPredicted`.

(The calculations can also manually be done via the Blueprint thread-safe functions `PredictGroundedStopLocation` and `PredictGroundedPivotLocation`.)

If `Draw Debug Predictions` is enabled with precalculations enabled, the component will draw predicted stop and pivot points; predicted pivot points are yellow (turning to white at the final prediction), while predicted stop points are blue (turning to black at the final prediction).

### Trajectory Prediction

If `Trajectory Enabled` is true, the component will keep historical samples; if `Precalculate Future Trajectory` is also true, the component will keep a constantly-updated version of the predicted trajectory available in `Predicted Trajectory`.

There's also an `FGMCE_MovementSample` which serves as the data storage container for the trajectory, and which can be cast into a stock Unreal `FTrajectorySample` as-needed, as well as an `FGMCE_MovementSampleCollection` which can similarly be cast into an `FTrajectorySampleRange`. For blueprint use, there are also two blueprint functions provided to turn the GMCEx structures into standard Epic Motion Trajectory ones.

If `Draw Debug Predictions` is enabled with precalculations enabled, the component will draw a pathway showing historical movement and predicted trajectory.

### Ragdolling

***Note:** This functionality was hastily written and could definitely be improved on. Maybe by you! Feel free to fork, modify, and make a merge request!*

This component uses the GMC `Custom1` movement mode for ragdolling logic; `Enable Ragdoll` and `Disable Ragdoll` will toggle the character into ragdoll mode or back to normal grounded movement.

For purposes of network sync, the character's position will remain unchanged when ragdolling; the skeletal mesh will be thrown (preserving velocity) as a ragdoll, but upon disabling ragdoll mode, the mesh will snap back to the position it was in at the time of ragdoll.

(Since ragdolling is usually used on death, before respawning a character, this didn't seem a limitation worth spending a ton of time to solve.)

It's worth noting that in the common setup with a spring arm and camera attached to the collision capsule, the camera will NOT follow the ragdolled mesh. (There are ways to solve that, but they're left as a game-specific exercise to the user.)

## UGMCE_UtilityLibrary

A blueprint function library providing some useful little oft-used calculations, such as calculating the angle of difference between two trajectories and returning a value in the range of -180 to 180 degrees.