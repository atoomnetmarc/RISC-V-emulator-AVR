/*
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef Snapshot_H_
#define Snapshot_H_

#if (RVE_AVR_LINUX == 1)

/**
 * Handles a pending host snapshot request.
 */
void snapshot_poll(void);

#endif

#endif
