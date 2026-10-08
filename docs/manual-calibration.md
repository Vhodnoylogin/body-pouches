# Manual slot calibration and repeatable interaction

The pouch uses the position and rotation of its VRIK slot, including an existing
weapon's rotation. Slot 13 is attached to `NPC Spine1 [Spn1]`, not the pelvis.
An arbitrary bone-space offset or 90-degree rotation is not a verified fit.
The owner reported the installed 0.1.16 bottle looked attached base-first to the
body and asked for a lower belt/hip placement. There is no screenshot of that run.

During a separately authorized headset session:

1. Preserve the actual winning `vrikslots.ini` and record its hash. Identify the
   pouch's actual slot and prevent another nearby holster from being mistaken for it.
2. Use VRIK's holster adjustment mode with an empty hand. With holsters unlocked,
   grip the pouch slot and hold for the native reposition pulse (about two seconds).
   Move the slot toward the owner's chosen lower belt/hip position and rotate the
   wrist until the bottle's neck points upward beside the body. Release to save.
   Do this while looking at the actual bottle; no fixed rotation matrix is prescribed.
3. Respect VRIK's movement limit (35 game units from the slot bone). If the desired
   lower location exceeds it, stop and choose a suitable lower slot in a separately
   prepared configuration. Do not repeatedly force/remove the existing slot.
4. Lock holsters through VRIK's controls once calibrated. This is a global VRIK
   preference, so the owner/operator must set it deliberately and record it. Body
   Pouches never silently overwrites it. Preserve the resulting config for the test.
5. Capture a real headset/mirror image showing the bottle at the new position,
   including neck and base. Verify the saved image; a capture receipt is insufficient.

VRIK owns the hover vibration. It is an entry pulse, not continuous feedback or
proof of a completed stow. VRIK keeps a per-hand pulsed state and biases distance
by 0.8 while that state is active. A small out-and-back motion can therefore stay
inside the same effective hover interval and produce no second pulse. A native
grip lock can also keep selection on a slot. This describes the implementation;
it does not prove the reason for the owner's missed entries.

Record actual native slot/distance, body/hand pose and vibration separately during
repeated approaches. Include an unmistakable complete exit, a small boundary
motion, an adjacent holster, long grip and a held potion. If native reach enters
the pouch anew but no haptic occurs, retain that as a separate VRIK/input finding.
Do not count missing vibration alone as a failed inventory transfer, or one
successful transfer as reliable recognition of every approach.

To stow, release the held potion inside the slot. To draw, begin with an empty
hand, press grip inside and withdraw while held. Merely approaching while holding
the bottle neither stows nor draws. Candidate 0.1.17 fixes only release selection:
the release/reference is resolved once after VRIK in its own HIGGS update. A
missing post callback, loading, either-hand regrab or an unrelated native holster
cancels it. It does not change VRIK's haptic rearm logic or increase the radius.
