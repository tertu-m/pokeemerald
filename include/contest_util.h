#ifndef GUARD_CONTEST_UTIL_H
#define GUARD_CONTEST_UTIL_H

extern u32 gLinkContestCompatRngValue;

void BufferContestantTrainerName(void);
void BufferContestantMonNickname(void);
void StartContest(void);
void BufferContestantMonSpecies(void);
void ShowContestResults(void);
void ContestLinkTransfer(u8);
void ShowContestPainting(void);
u16 GetContestRand(void);
u8 CountPlayerMuseumPaintings(void);
u16 ContestCompatRandom(void);

#endif // GUARD_CONTEST_UTIL_H
