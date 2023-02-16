#include "rtc_queue.h"
#include "rtc.h"
#include "event_data.h"
#include "script.h"

struct RtcTriggerEntry {
    u32 triggerTime;
    const u8* triggerScript;
};

/*
There are two trigger types: immediate triggers and interactive triggers.
Immediate triggers are run as soon as possible, but cannot do certain actions.
Interactive triggers will be run as soon as the main context is available and
can perform any functionality that is desired.
Special var 0x8014, normally unused, is used to pass how much time has elapsed
since a script's desired trigger time.
As immediate scripts and interactive scripts both use this special var to pass
this value, interactive scripts should copy the contents of 0x8014 into their
data if you also use immediate scripts.
*/

static EWRAM_DATA struct RtcTriggerEntry interactiveTriggers[32];
static EWRAM_DATA u8 interactiveTriggerCount;
static EWRAM_DATA struct RtcTriggerEntry immediateTriggers[16];
static EWRAM_DATA u8 immediateTriggerCount;
static EWRAM_DATA u32 lastUpdateTime;

union UnsignedSigned {
    s32 _signed;
    u32 _unsigned;
};

static u32 GetRtcSeconds()
{
    struct SiiRtcInfo rtc;
    u32 days;
    RtcGetDateTime(&rtc);
    days = RtcGetDayCount(&rtc);
    return ((days * 24 + rtc.hour) * 60 + rtc.minute) * 60 + rtc.second;
}

// Returns how many children this entry has.
static u8 HeapGetChildIndices(u8 size, u8 index, u8 *child1, u8 *child2)
{
    if (size == 0 || size < index * 2)
        return 0;
    *child1 = index * 2 + 1;
    *child2 = index * 2 + 2;

    if (size == index * 2)
        return 1;
    else
        return 2;
}

static inline void SwapEntries(struct RtcTriggerEntry *a, struct RtcTriggerEntry *b)
{
    struct RtcTriggerEntry temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

// The sift-down operation moves an entry "downwards" into its proper place.
static void HeapSiftDown(struct RtcTriggerEntry heap[], u8 size, u8 index)
{
    u8 child1, child2, children;
    u32 child1Time, child2Time, childTime, curTime;

    // this is invariant; we'll always be processing the same entry as it moves
    curTime = heap[index].triggerTime;

    while (index < size) // shouldn't ever happen but just in case.
    {
        children = HeapGetChildIndices(size, index, &child1, &child2);
        switch (children){
            case 0:
            default:
                return;
            case 1:
                child1Time = heap[child1].triggerTime;
                if (child1Time < curTime)
                {
                    SwapEntries(&heap[index], &heap[child1]);
                    index = child1;
                }
                else
                    return;
                break;
            case 2:
                child1Time = heap[child1].triggerTime;
                child2Time = heap[child2].triggerTime;
                if (child1Time < curTime && child1Time <= child2Time)
                {
                    SwapEntries(&heap[index], &heap[child1]);
                    index = child1;
                }
                else if (child2Time < curTime)
                {
                    SwapEntries(&heap[index], &heap[child2]);
                    index = child2;
                }
                else
                    return;
                break;
        }
    }
}

// The sift-up operation moves an entry "upwards" into its proper place.
static void HeapSiftUp(struct RtcTriggerEntry heap[], u8 index)
{
    u8 parent;
    u32 curTime, parentTime;
    curTime = heap[index].triggerTime;
    while (index > 0)
    {
        parent = (index-1) >> 1;
        parentTime = heap[parent].triggerTime;
        if (parentTime > curTime)
        {
            SwapEntries(&heap[index], &heap[parent]);
            index = parent;
        }
        else
            return;
    }
}

static u8 RemoveFirstEntry(struct RtcTriggerEntry heap[], u8 *size)
{
    u8 tempSize;
    tempSize = *size;
    if (tempSize <= 1)
        tempSize = 0;
    else
    {
        tempSize--;
        heap[0] = heap[tempSize];
        HeapSiftDown(heap, tempSize, 0);
    }
    *size = tempSize;
    return tempSize;
}

// DOES NOT CHECK for overfill!
// The caller must do this.
static u8 AddEntry(struct RtcTriggerEntry heap[], u8 *size, struct RtcTriggerEntry *entry)
{
    u8 tempSize;
    tempSize = *size;
    heap[tempSize] = *entry;
    HeapSiftUp(heap, tempSize);
    tempSize++;
    *size = tempSize;
    return tempSize;
}

static void RebuildHeap(struct RtcTriggerEntry heap[], u8 size)
{
    if (size > 1)
    {
        u32 i;
        for (i = size/2; i > 0; i--)
            HeapSiftDown(heap, size, i);
    }
}

/*
Not currently needed

static u8 RemoveArbitraryEntry(struct RtcTriggerEntry heap[], u8 size, u8 index)
{
    u32 oldTime, newTime;
    if (index >= size)
        return size;
    else if (index == size-1)
        return size-1;
    else if (size <= 1)
        return 0;

    oldTime = heap[index].triggerTime;
    newTime = heap[size-1].triggerTime;
    SwapEntries(&heap[index], &heap[size-1]);
    if(oldTime < newTime)
        HeapSiftUp(heap, index);
    else if (oldTime > newTime)
        HeapSiftDown(heap, size, index);

    return size-1;
}
*/

// The idea of unsigned_abs seems weird, but the implementation suggests what it does
// Return what the absolute value of a u32 would be if it were an s32.
static inline u32 unsigned_abs(u32 x)
{
    s32 mask, signed_x;
    union UnsignedSigned pun;

    pun._unsigned = x;
    signed_x = pun._signed;
    mask = signed_x >> 31;

    pun._signed = (mask + signed_x) ^ mask;
    return pun._unsigned;
}

// Returns the sign bit: 1 if negative, 0 if positive.
// Again this is treating a u32 like an s32.
static inline bool8 sign(u32 value)
{
    return value >> 31;
}

// Handle the RTC moving backwards in time.
// This is done by assuming that all triggers should move backwards by that change.
static void HandleBackwards(struct RtcTriggerEntry heap[], u8 size, u32 change)
{
    u8 i;
    // This doesn't reorder the heap. The same change is applied to every entry.
    for (i=0; i < size; i++)
        heap[i].triggerTime -= change;
}

// Handle the RTC overflowing to 0.
static void HandleOverflow(struct RtcTriggerEntry heap[], u8 size)
{
    u8 i;
    for (i=0; i < size; i++)
    {
        if (sign(heap[i].triggerTime) == 1)
            heap[i].triggerTime = 0;
    }
    RebuildHeap(heap, size);
}

// The task for handling RTC triggers.
void Task_HandleRtcTriggers(u8 taskId)
{
    u8 i;
    u32 now, change, triggerTime, difference;

    // The RTC isn't set up yet; triggers can't happen.
    if (!FlagGet(FLAG_SYS_CLOCK_SET))
        return;

    now = GetRtcSeconds();
    change = now - lastUpdateTime;
    // The RTC is the same as it was last time, no change needed.
    if (change == 0)
        return;

    difference = 0;

    // The high 4 bits of the max RTC second value are 11.
    // If the high 4 bits of the RTC second value went from 11 to 0, that
    // means the RTC overflowed.
    if ((now >> 28) == 0 && (lastUpdateTime >> 28) == 11)
    {
        HandleOverflow(interactiveTriggers, interactiveTriggerCount);
        HandleOverflow(immediateTriggers, immediateTriggerCount);
    }
    else if (sign(change) == 1)
    {
        u32 amount;
        amount = unsigned_abs(change);
        HandleBackwards(interactiveTriggers, interactiveTriggerCount, amount);
        HandleBackwards(immediateTriggers, immediateTriggerCount, amount);
    }

    while (immediateTriggerCount > 0 && sign(difference) == 0)
    {
        triggerTime = immediateTriggers[0].triggerTime;
        difference = now - triggerTime;
        if (sign(difference) == 0)
        {
            // As 0x8014 is unused, using it cannot interrupt any running
            // vanilla scripts.
            // Note that interactive trigger scripts can be disrupted, so they
            // should begin by copying the contents of 0x8014 into one of their
            // data values, as stated above.
            gSpecialVar_Unused_0x8014 = difference;
            RunScriptImmediately(immediateTriggers[0].triggerScript);
            RemoveFirstEntry(immediateTriggers, &immediateTriggerCount);
        }
    }

    difference = 0;

    while (interactiveTriggerCount > 0 && ScriptContext_IsIdle()
        && sign(difference) == 0)
    {
        triggerTime = interactiveTriggers[0].triggerTime;
        difference = now - triggerTime;
        if (sign(difference) == 0)
        {
            gSpecialVar_Unused_0x8014 = difference;
            ScriptContext_SetupScript(interactiveTriggers[0].triggerScript);
            RemoveFirstEntry(interactiveTriggers, &interactiveTriggerCount);
        }
    }
    lastUpdateTime = now;
}

bool8 AddRtcTrigger(u16 secondsFromNow, const u8 *script, bool8 immediate)
{
    u32 time;
    time = GetRtcSeconds() + secondsFromNow;
    return AddRtcTriggerAtTime(time, script, immediate);
}

bool8 AddRtcTriggerAtTime(u32 time, const u8 *script, bool8 immediate)
{
    struct RtcTriggerEntry newEntry;
    struct RtcTriggerEntry *heap;
    u8 heapLimit;
    u8 *heapCount;

    if (immediate)
    {
        heap = (struct RtcTriggerEntry *)&immediateTriggers;
        heapLimit = NELEMS(immediateTriggers);
        heapCount = &immediateTriggerCount;
    }
    else
    {
        heap = (struct RtcTriggerEntry *)&interactiveTriggers;
        heapLimit = NELEMS(interactiveTriggers);
        heapCount = &interactiveTriggerCount;
    }

    if (*heapCount >= heapLimit)
        return FALSE;

    newEntry.triggerTime = time;
    newEntry.triggerScript = script;
    AddEntry(heap, heapCount, &newEntry);

    return TRUE;
}