#include "ss3ds_pwalker_ir.h"
#include <3ds.h>
#include <stdatomic.h>
#include <string.h>

extern void PSPNativeMemLog(const char *fmt, ...);
extern int P3DS_CfgInt(const char *key, int fallback);

// The original overlay holds its own mutex while polling these functions.
// No system service call may run with either the overlay mutex or busLock held.
enum { PacketCapacity = 136, TxCapacity = 4 };
typedef struct {
    size_t length;
    uint8_t bytes[PacketCapacity];
} Packet;

static LightLock busLock;
static LightEvent busWake;
static bool busLockInitialized;
static Thread busThread;
static bool active;
static bool tracePackets;
static bool restoreInput;
static bool ready;
static bool startupReported;
static u32 generation;
static u64 openedAt;
static Packet incoming;
static Packet outgoing[TxCapacity];
static unsigned txHead;
static unsigned txCount;

static int receivePacket(Packet *packet)
{
    int count = SS3DS_PwalkerIrRead(packet->bytes, 1);
    if (count <= 0)
        return count;
    // The one-byte discovery signal has no header. The DS code accepts
    // it only when it is delivered alone, even if the UART has more bytes.
    if (packet->bytes[0] == 0x56) {
        packet->bytes[0] = 0xfc;
        packet->length = 1;
        return 1;
    }
    size_t length = 1;
    u64 lastByte = svcGetSystemTick();
    while (length < sizeof packet->bytes) {
        count = SS3DS_PwalkerIrRead(packet->bytes + length,
                                    sizeof packet->bytes - length);
        if (count < 0)
            return -1;
        if (count) {
            length += (size_t)count;
            lastByte = svcGetSystemTick();
        } else if (svcGetSystemTick() - lastByte >= SYSCLOCK_ARM11 / 500) {
            break;
        }
    }
    for (size_t i = 0; i < length; i++)
        packet->bytes[i] ^= 0xaa; // Cartridge receive decoded the wire bytes.
    packet->length = length;
    return (int)length;
}

static bool validPacket(const Packet *packet)
{
    if (packet->length < 8)
        return false;
    u32 sum = 2;
    for (size_t i = 0; i < packet->length; i += 2) {
        if (i == 2)
            continue;
        sum += (u32)packet->bytes[i] << 8;
        if (i + 1 < packet->length)
            sum += packet->bytes[i + 1];
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return sum == (packet->bytes[2] | (u32)packet->bytes[3] << 8);
}

static void busWorker(void *unused)
{
    (void)unused;
    u32 session = 0;
    bool listening = false;
    unsigned txLogged = 0;
    unsigned rxLogged = 0;
    unsigned receiveErrors = 0;
    for (;;) {
        Packet packet;
        bool send = false;

        LightLock_Lock(&busLock);
        u32 requested = generation;
        bool wanted = active;
        bool trace = tracePackets;
        bool shouldRestore = restoreInput && !wanted && !txCount;
        if (shouldRestore)
            restoreInput = false;
        bool canReceive = incoming.length == 0;
        if (listening && session == requested && txCount) {
            packet = outgoing[txHead];
            txHead = (txHead + 1) % TxCapacity;
            txCount--;
            send = true;
        }
        LightLock_Unlock(&busLock);

        if (session != requested || (!wanted && !send && listening)) {
            SS3DS_PwalkerIrClose();
            listening = false;
        }
        if (shouldRestore)
            SS3DS_PwalkerIrRestoreInput();
        if (wanted && session != requested) {
            session = requested;
            txLogged = rxLogged = 0;
            receiveErrors = 0;
            listening = SS3DS_PwalkerIrOpen() && SS3DS_PwalkerIrListen();
            if (!listening)
                SS3DS_PwalkerIrClose();
            LightLock_Lock(&busLock);
            if (generation == session && active)
                ready = listening;
            LightLock_Unlock(&busLock);
            PSPNativeMemLog("[PWALKER] %s", listening ? "IR ready" : "IR initialization failed");
        } else if (send) {
            bool sent = SS3DS_PwalkerIrSend(packet.bytes, packet.length);
            if (!sent && !SS3DS_PwalkerIrListen()) {
                SS3DS_PwalkerIrClose();
                listening = false;
            }
            if (sent && listening && trace && txLogged < 160) {
                txLogged++;
                PSPNativeMemLog("[PWALKER] TX complete: opcode %02x, %u bytes",
                                (unsigned)(packet.bytes[0] ^ 0xaa),
                                (unsigned)packet.length);
            }
            if (!sent || !listening)
                PSPNativeMemLog("[PWALKER] IR transmit failed: opcode %02x, %u bytes, sent=%d rx-ready=%d",
                                (unsigned)(packet.bytes[0] ^ 0xaa),
                                (unsigned)packet.length, sent, listening);
        } else if (listening && wanted && canReceive) {
            int count = receivePacket(&packet);
            if (count >= 0)
                receiveErrors = 0;
            if (count > 0) {
                const bool valid = packet.length == 1 || validPacket(&packet);
                if (valid) {
                    LightLock_Lock(&busLock);
                    if (active && generation == session)
                        incoming = packet;
                    LightLock_Unlock(&busLock);
                }
                if (!valid) {
                    int status = SS3DS_PwalkerIrLineStatus();
                    PSPNativeMemLog("[PWALKER] RX line status: %d (overrun=%d)",
                                    status, status >= 0 && (status & BIT(1)));
                }
                if (packet.length > 1 && (!valid || (trace && rxLogged < 160))) {
                    rxLogged++;
                    PSPNativeMemLog("[PWALKER] RX %s: opcode %02x, %u bytes",
                                    valid ? "valid" : "invalid",
                                    (unsigned)packet.bytes[0], (unsigned)packet.length);
                }
            } else if (count < 0) {
                // A failed FIFO read can consume part of a packet. Discard
                // it and re-arm RX rather than replaying the read.
                if (++receiveErrors < 3 && SS3DS_PwalkerIrListen()) {
                    PSPNativeMemLog("[PWALKER] IR receive re-armed (%u/2)",
                                    receiveErrors);
                } else {
                    PSPNativeMemLog("[PWALKER] IR receive failed; closing session");
                    SS3DS_PwalkerIrClose();
                    listening = false;
                }
            }
        }
        if (!listening) {
            LightLock_Lock(&busLock);
            if (generation == session)
                ready = false;
            LightLock_Unlock(&busLock);
        }
        if (wanted || send)
            LightEvent_WaitTimeout(&busWake, 1000000);
        else
            LightEvent_Wait(&busWake);
    }
}

uint32_t SS3DS_Pwalker_SpiWaitInit(void)
{
    if (!busLockInitialized) {
        LightLock_Init(&busLock);
        LightEvent_Init(&busWake, RESET_ONESHOT);
        busLockInitialized = true;
    }
    if (!busThread) {
        // One process-lifetime thread. A blocked IPC cannot hold up overlay
        // teardown, and reconnects do not create additional blocked threads.
        // Drain the small RX FIFO ahead of normal rendering workers.
        busThread = threadCreate(busWorker, NULL, 32 * 1024, 0x2d, -2, false);
        if (!busThread) {
            PSPNativeMemLog("[PWALKER] could not start IR worker");
            return 0;
        }
    }

    bool trace = P3DS_CfgInt("pokewalker_trace", 0) != 0;
    LightLock_Lock(&busLock);
    generation++;
    active = true;
    tracePackets = trace;
    restoreInput = false;
    ready = false;
    startupReported = false;
    openedAt = svcGetSystemTick();
    incoming.length = 0;
    txHead = txCount = 0;
    LightLock_Unlock(&busLock);
    LightEvent_Signal(&busWake);
    PSPNativeMemLog("[PWALKER] queued IR initialization");
    return 0;
}

uint32_t SS3DS_Pwalker_SpiWaitGetStatus(void)
{
    if (!busThread)
        return 0;
    LightLock_Lock(&busLock);
    bool listening = ready;
    LightLock_Unlock(&busLock);
    return listening ? 0xaa : 0;
}

// These delays apply to the original cartridge SPI bus.
uint32_t SS3DS_Pwalker_SetSpiWriteWaitCycles(uint32_t cycles)
{
    (void)cycles;
    return 0;
}

uint32_t SS3DS_Pwalker_SetSpiReadWaitCycles(uint32_t cycles)
{
    (void)cycles;
    return 0;
}

uint32_t SS3DS_Pwalker_SpiWaitReadRange(uint8_t *buffer)
{
    if (!busThread)
        return 0;
    LightLock_Lock(&busLock);
    size_t length = incoming.length;
    if (length)
        memcpy(buffer, incoming.bytes, length);
    incoming.length = 0;
    bool late = active && !ready && !startupReported &&
                svcGetSystemTick() - openedAt > SYSCLOCK_ARM11;
    if (late)
        startupReported = true;
    LightLock_Unlock(&busLock);
    if (late)
        PSPNativeMemLog("[PWALKER] IR not ready after 1 s; game remains responsive");
    return (uint32_t)length;
}

uint32_t SS3DS_Pwalker_SpiWaitWriteRange(const uint8_t *buffer, uint32_t length)
{
    if (!busThread || !length || length > PacketCapacity)
        return 0;
    LightLock_Lock(&busLock);
    bool queued = active && ready && txCount < TxCapacity;
    if (queued) {
        Packet *packet = &outgoing[(txHead + txCount) % TxCapacity];
        packet->length = length;
        memcpy(packet->bytes, buffer, length);
        txCount++;
    }
    LightLock_Unlock(&busLock);
    if (queued)
        LightEvent_Signal(&busWake);
    else
        PSPNativeMemLog("[PWALKER] IR packet could not be queued");
    return 0;
}

// Stop the worker cooperatively. The overlay requests shutdown with its
// mutex held, then releases that mutex before joining the worker.

extern void OS_ExitThread(void);
extern uint32_t SS3DS_Pwalker_UnlockMutex(void);
extern void SS3DS_Pwalker_JoinWorker(void);
extern void OS_Sleep(uint32_t milliseconds);

static atomic_bool stopping;

uint32_t SS3DS_Pwalker_RequestStop(void)
{
    if (busThread) {
        LightLock_Lock(&busLock);
        active = false;
        ready = false;
        incoming.length = 0;
        LightLock_Unlock(&busLock);
        LightEvent_Signal(&busWake);
    }
    atomic_store(&stopping, true);
    return 0;
}

uint32_t SS3DS_Pwalker_UnlockAndJoin(void)
{
    SS3DS_Pwalker_UnlockMutex();
    SS3DS_Pwalker_JoinWorker();
    atomic_store(&stopping, false);
    return 0;
}

uint32_t SS3DS_Pwalker_WorkerSleep(uint32_t milliseconds)
{
    if (atomic_load(&stopping))
        OS_ExitThread();
    OS_Sleep(milliseconds);
    return 0;
}

int SS3DS_PwalkerCanLaunch(void)
{
    return P3DS_CfgInt("pokewalker", 0) != 0;
}

uint32_t SS3DS_Pwalker_Idle(void)
{
    OS_Sleep(1);
    return 0;
}

extern void Heap_Destroy(uint32_t heap);

uint32_t SS3DS_Pwalker_DestroyAppHeap(uint32_t heap)
{
    if (busThread) {
        LightLock_Lock(&busLock);
        restoreInput = true;
        LightLock_Unlock(&busLock);
        LightEvent_Signal(&busWake);
    }
    Heap_Destroy(heap);
    return 0;
}
