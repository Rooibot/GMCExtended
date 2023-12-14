# GMCv2 Extended

This repository contains a simple plugin which extends GMCv2 in various ways, with utilities and such provided by GMC licensees.

(Mostly it exists because Packetdancer got tired of answering how to copy the trajectory movement component into Blueprint projects, and her solution was "just turn it into a plugin".)

There will be better documentation here eventually. Probably.

Things currently in the project:

## UGMCE_OrganicMovementCmp

This is an extension of GMCv2's Organic Movement Component, adding a few things:

* Stop/pivot prediction (for distance matching)
* Trajectory prediction (for motion trajectory)
* Debug visualization options for the above two
* A very, _very_ basic ragdoll mode

There's also an `FGMCE_MovementSample` which serves as the data storage container for the trajectory, and which can be cast into a stock Unreal `FTrajectorySample` as-needed.

## UGMCE_UtilityLibrary

A blueprint function library providing some useful little oft-used calculations, such as calculating the angle of difference between two trajectories and returning a value in the range of -180 to 180 degrees.