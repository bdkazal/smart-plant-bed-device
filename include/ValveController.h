#pragma once

void openFakeValve();
void closeFakeValve();

void startWateringCommand(int commandId, int durationSeconds);
void stopWateringCommand(int commandId);

void updateWateringState();

bool isValveOpen();
bool isWateringActive();
int getActiveCommandId();

void rememberPendingCompletedCommand(int commandId);
void loadPendingCompletedCommandFromStorage();
bool hasPendingCompletedCommand();
int getPendingCompletedCommandId();
void clearPendingCompletedCommand();
bool syncPendingCompletedCommandIfNeeded();
