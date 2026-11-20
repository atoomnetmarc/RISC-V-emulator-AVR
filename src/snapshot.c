/*
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 */

#if (RVE_AVR_E_LINUX == 1)

#include "snapshot.h"

#include <stddef.h>
#include <string.h>

#define RVE_SNAPSHOT_BLOB_ONLY 1
#include <RiscvEmulatorSnapshot.h>
#include <RiscvEmulatorTypeCSR.h>

#include "linux.h"
#include "vpi.h"

extern RiscvEmulatorState_t RiscvEmulatorState;
extern uint64_t RiscvEmulatorInstructionCount;

extern syscon_t syscon;
extern clint_t clint;
extern uart16550_t uart;
extern plic_t plic;
extern virtio_blk_t virtio_blk;
extern virtio_console_t virtio_console;
extern virtio_net_t virtio_net;
extern virtio_rng_t virtio_rng;
extern goldfish_rtc_t goldfish_rtc;

#define SNAPSHOT_MAGIC 0x5256534Eu

/**
 * Copies a 32-bit value into a packed CSR field by byte offset.
 */
#if (RVE_E_ZICSR == 1)
static void csr_set_u32(size_t offset, uint32_t value) {
    memcpy((uint8_t *)&RiscvEmulatorState.csr + offset, &value, sizeof(value));
}

#if (RVE_E_ZICNTR == 1)
/**
 * Copies a 64-bit value into a packed CSR field by byte offset.
 */
static void csr_set_u64(size_t offset, uint64_t value) {
    memcpy((uint8_t *)&RiscvEmulatorState.csr + offset, &value, sizeof(value));
}
#endif
#endif

/**
 * Copies one virtio-mmio transport into the snapshot blob.
 */
static void pack_mmio(rve_snapshot_mmio_t *blob, const virtio_mmio_t *mmio) {
    blob->base = mmio->base;
    blob->size = mmio->size;
    blob->memory_base = mmio->memory_base;
    blob->memory_size = mmio->memory_size;
    blob->device_id = mmio->device_id;
    blob->vendor_id = mmio->vendor_id;
    blob->host_features = mmio->host_features;
    blob->host_features_sel = mmio->host_features_sel;
    blob->guest_features = mmio->guest_features;
    blob->guest_features_sel = mmio->guest_features_sel;
    blob->guest_page_size = mmio->guest_page_size;
    blob->queue_sel = mmio->queue_sel;
    blob->queue_count = mmio->queue_count;
    for (uint8_t i = 0; i < 2; i++) {
        blob->queue_num[i] = mmio->queues[i].queue_num;
        blob->queue_align[i] = mmio->queues[i].queue_align;
        blob->queue_pfn[i] = mmio->queues[i].queue_pfn;
        blob->last_avail_idx[i] = mmio->queues[i].last_avail_idx;
        blob->used_idx[i] = mmio->queues[i].used_idx;
    }
    blob->interrupt_status = mmio->interrupt_status;
    blob->status = mmio->status;
}

/**
 * Copies one virtio-mmio transport out of the snapshot blob.
 */
static void unpack_mmio(virtio_mmio_t *mmio, const rve_snapshot_mmio_t *blob) {
    mmio->base = blob->base;
    mmio->size = blob->size;
    mmio->memory_base = blob->memory_base;
    mmio->memory_size = blob->memory_size;
    mmio->device_id = blob->device_id;
    mmio->vendor_id = blob->vendor_id;
    mmio->host_features = blob->host_features;
    mmio->host_features_sel = blob->host_features_sel;
    mmio->guest_features = blob->guest_features;
    mmio->guest_features_sel = blob->guest_features_sel;
    mmio->guest_page_size = blob->guest_page_size;
    mmio->queue_sel = blob->queue_sel;
    mmio->queue_count = blob->queue_count;
    for (uint8_t i = 0; i < 2; i++) {
        mmio->queues[i].queue_num = blob->queue_num[i];
        mmio->queues[i].queue_align = blob->queue_align[i];
        mmio->queues[i].queue_pfn = blob->queue_pfn[i];
        mmio->queues[i].last_avail_idx = blob->last_avail_idx[i];
        mmio->queues[i].used_idx = blob->used_idx[i];
    }
    mmio->interrupt_status = blob->interrupt_status;
    mmio->status = blob->status;
}

/**
 * Fills the snapshot blob from the current CPU and device state.
 */
static void snapshot_pack(rve_snapshot_t *blob) {
    memset(blob, 0, sizeof(*blob));
    blob->magic = RVE_SNAPSHOT_MAGIC;
    blob->version = RVE_SNAPSHOT_VERSION;
    blob->instructions = RiscvEmulatorInstructionCount;
    blob->ram_origin = RVE_SNAPSHOT_RAM_ORIGIN;
    blob->ram_size = RVE_SNAPSHOT_RAM_SIZE;
    blob->isa_m = RVE_E_M;
    blob->isa_a = RVE_E_A;
    blob->isa_c = RVE_E_C;
    blob->isa_zicsr = RVE_E_ZICSR;
    blob->isa_zicntr = RVE_E_ZICNTR;
    blob->isa_sv32 = RVE_E_SV32;
    blob->isa_zalrsc = RVE_E_ZALRSC;

    blob->cpu_pc = RiscvEmulatorState.programcounter;
    blob->cpu_next_pc = RiscvEmulatorState.programcounternext;
    blob->cpu_instruction = RiscvEmulatorState.instruction.value;
    blob->cpu_instruction_executed = RiscvEmulatorState.instructionexecuted;
    blob->trap_pending = RiscvEmulatorState.trap.pending;
    blob->trap_cause = RiscvEmulatorState.trap.cause;
    blob->trap_tval = RiscvEmulatorState.trap.tval;
    for (uint8_t i = 0; i < 32; i++) {
        blob->reg[i] = RiscvEmulatorState.reg.x[i];
    }
#if (RVE_E_ZALRSC == 1)
    blob->cpu_reservation_valid = RiscvEmulatorState.reservationvalid;
    blob->cpu_reservation_address = RiscvEmulatorState.reservationaddress;
#endif
#if (RVE_E_HOOK == 1 || RVE_E_DISASM == 1)
    blob->cpu_instruction_handled = RiscvEmulatorState.instructionhandled;
#endif
#if (RVE_E_ZICSR == 1)
    blob->cpu_privilege = RiscvEmulatorState.privilege;
    blob->cpu_waiting = RiscvEmulatorState.waiting;
    blob->cpu_wfi_timeout = RiscvEmulatorState.wfi_timeout;
#if (RVE_E_ZICNTR == 1)
    blob->cpu_counter_written = RiscvEmulatorState.counterwritten;
#endif
#define PACK_CSR32(field)                                                                      \
    do {                                                                                       \
        uint32_t value;                                                                        \
        memcpy(&value, (const uint8_t *)&RiscvEmulatorState.csr + offsetof(RiscvCSR_t, field), \
               sizeof(value));                                                                 \
        blob->csr_##field = value;                                                             \
    } while (0)
    PACK_CSR32(mhartid);
    PACK_CSR32(marchid);
    PACK_CSR32(mstatus);
    PACK_CSR32(misa);
    PACK_CSR32(medeleg);
    PACK_CSR32(mideleg);
    PACK_CSR32(mie);
    PACK_CSR32(mtvec);
    PACK_CSR32(mstatush);
    PACK_CSR32(menvcfg);
    PACK_CSR32(mscratch);
    PACK_CSR32(mepc);
    PACK_CSR32(mcause);
    PACK_CSR32(mtval);
    PACK_CSR32(mip);
    PACK_CSR32(mip_platform);
    PACK_CSR32(mip_platform_software);
    PACK_CSR32(pmpcfg0);
    PACK_CSR32(pmpaddr0);
    PACK_CSR32(mnstatus);
    PACK_CSR32(stvec);
    PACK_CSR32(scounteren);
    PACK_CSR32(senvcfg);
    PACK_CSR32(sscratch);
    PACK_CSR32(sepc);
    PACK_CSR32(scause);
    PACK_CSR32(stval);
    PACK_CSR32(satp);
    PACK_CSR32(zeroreg);
    PACK_CSR32(mcounteren);
#undef PACK_CSR32
#if (RVE_E_ZICNTR == 1)
#define PACK_CSR64(field)                                                                      \
    do {                                                                                       \
        uint64_t value;                                                                        \
        memcpy(&value, (const uint8_t *)&RiscvEmulatorState.csr + offsetof(RiscvCSR_t, field), \
               sizeof(value));                                                                 \
        blob->csr_##field = value;                                                             \
    } while (0)
    PACK_CSR64(cycle);
    PACK_CSR64(time);
    PACK_CSR64(instret);
    PACK_CSR64(mhpmcounter3);
#undef PACK_CSR64
#endif
#endif

    blob->clint_mtime = clint.mtime;
    blob->clint_mtimecmp = clint.mtimecmp;
    blob->clint_last_us = clint.last_us;
    blob->clint_msip = clint.msip;
    blob->clint_mtime_high_latched = clint.mtime_high_latched;
    blob->clint_mtime_high_latch_valid = clint.mtime_high_latch_valid;
    for (uint8_t i = 0; i < PLIC_SOURCE_COUNT; i++) {
        blob->plic_priority[i] = plic.priority[i];
    }
    blob->plic_pending = plic.pending;
    for (uint8_t i = 0; i < PLIC_CONTEXT_COUNT; i++) {
        blob->plic_enable[i] = plic.enable[i];
        blob->plic_threshold[i] = plic.threshold[i];
    }
    blob->uart_ier = uart.ier;
    blob->uart_fcr = uart.fcr;
    blob->uart_lcr = uart.lcr;
    blob->uart_mcr = uart.mcr;
    blob->uart_scr = uart.scr;
    blob->uart_dll = uart.dll;
    blob->uart_dlm = uart.dlm;
    memcpy(blob->uart_fifo, uart.fifo, UART16550_FIFO_SIZE);
    blob->uart_fifo_read = uart.fifo_read;
    blob->uart_fifo_write = uart.fifo_write;
    blob->uart_fifo_count = uart.fifo_count;
    blob->uart_overrun = uart.overrun;
    blob->uart_thre_pending = uart.thre_pending;
    pack_mmio(&blob->virtio[0], &virtio_blk.mmio);
    blob->virtio_blk_capacity = virtio_blk.capacity;
    pack_mmio(&blob->virtio[1], &virtio_console.mmio);
    memcpy(blob->virtio_console_rx_fifo, virtio_console.rx_fifo, VIRTIO_CONSOLE_RX_FIFO_SIZE);
    blob->virtio_console_rx_read = virtio_console.rx_read;
    blob->virtio_console_rx_write = virtio_console.rx_write;
    blob->virtio_console_rx_count = virtio_console.rx_count;
    blob->virtio_console_rx_overrun = virtio_console.rx_overrun;
    pack_mmio(&blob->virtio[2], &virtio_net.mmio);
    pack_mmio(&blob->virtio[3], &virtio_rng.mmio);
    blob->rtc_base_ns = goldfish_rtc.base_ns;
    blob->rtc_anchor_ns = goldfish_rtc.anchor_ns;
    blob->rtc_latch_high = goldfish_rtc.latch_high;
    blob->rtc_alarm_low = goldfish_rtc.alarm_low;
    blob->rtc_alarm_high = goldfish_rtc.alarm_high;
    blob->rtc_irq_enabled = goldfish_rtc.irq_enabled;
    blob->rtc_alarm_status = goldfish_rtc.alarm_status;
}

/**
 * Applies the snapshot blob to the CPU and device state.
 */
static void snapshot_unpack(const rve_snapshot_t *blob) {
    RiscvEmulatorInstructionCount = blob->instructions;
    RiscvEmulatorState.programcounter = blob->cpu_pc;
    RiscvEmulatorState.programcounternext = blob->cpu_next_pc;
    RiscvEmulatorState.instruction.value = blob->cpu_instruction;
    RiscvEmulatorState.instructionexecuted = blob->cpu_instruction_executed;
    RiscvEmulatorState.trap.pending = blob->trap_pending;
    RiscvEmulatorState.trap.cause = blob->trap_cause;
    RiscvEmulatorState.trap.tval = blob->trap_tval;
    for (uint8_t i = 0; i < 32; i++) {
        RiscvEmulatorState.reg.x[i] = blob->reg[i];
    }
#if (RVE_E_ZALRSC == 1)
    RiscvEmulatorState.reservationvalid = blob->cpu_reservation_valid;
    RiscvEmulatorState.reservationaddress = blob->cpu_reservation_address;
#endif
#if (RVE_E_HOOK == 1 || RVE_E_DISASM == 1)
    RiscvEmulatorState.instructionhandled = blob->cpu_instruction_handled;
#endif
#if (RVE_E_ZICSR == 1)
    RiscvEmulatorState.privilege = blob->cpu_privilege;
    RiscvEmulatorState.waiting = blob->cpu_waiting;
    RiscvEmulatorState.wfi_timeout = blob->cpu_wfi_timeout;
#if (RVE_E_ZICNTR == 1)
    RiscvEmulatorState.counterwritten = blob->cpu_counter_written;
#endif
#define UNPACK_CSR32(field) csr_set_u32(offsetof(RiscvCSR_t, field), blob->csr_##field)
    UNPACK_CSR32(mhartid);
    UNPACK_CSR32(marchid);
    UNPACK_CSR32(mstatus);
    UNPACK_CSR32(misa);
    UNPACK_CSR32(medeleg);
    UNPACK_CSR32(mideleg);
    UNPACK_CSR32(mie);
    UNPACK_CSR32(mtvec);
    UNPACK_CSR32(mstatush);
    UNPACK_CSR32(menvcfg);
    UNPACK_CSR32(mscratch);
    UNPACK_CSR32(mepc);
    UNPACK_CSR32(mcause);
    UNPACK_CSR32(mtval);
    UNPACK_CSR32(mip);
    UNPACK_CSR32(mip_platform);
    UNPACK_CSR32(mip_platform_software);
    UNPACK_CSR32(pmpcfg0);
    UNPACK_CSR32(pmpaddr0);
    UNPACK_CSR32(mnstatus);
    UNPACK_CSR32(stvec);
    UNPACK_CSR32(scounteren);
    UNPACK_CSR32(senvcfg);
    UNPACK_CSR32(sscratch);
    UNPACK_CSR32(sepc);
    UNPACK_CSR32(scause);
    UNPACK_CSR32(stval);
    UNPACK_CSR32(satp);
    UNPACK_CSR32(zeroreg);
    UNPACK_CSR32(mcounteren);
#undef UNPACK_CSR32
#if (RVE_E_ZICNTR == 1)
#define UNPACK_CSR64(field) csr_set_u64(offsetof(RiscvCSR_t, field), blob->csr_##field)
    UNPACK_CSR64(cycle);
    UNPACK_CSR64(time);
    UNPACK_CSR64(instret);
    UNPACK_CSR64(mhpmcounter3);
#undef UNPACK_CSR64
#endif
#endif

    clint.mtime = blob->clint_mtime;
    clint.mtimecmp = blob->clint_mtimecmp;
    clint.last_us = vpi_time_source();
    clint.msip = blob->clint_msip;
    clint.mtime_high_latched = blob->clint_mtime_high_latched;
    clint.mtime_high_latch_valid = blob->clint_mtime_high_latch_valid;
    for (uint8_t i = 0; i < PLIC_SOURCE_COUNT; i++) {
        plic.priority[i] = blob->plic_priority[i];
    }
    plic.pending = blob->plic_pending;
    for (uint8_t i = 0; i < PLIC_CONTEXT_COUNT; i++) {
        plic.enable[i] = blob->plic_enable[i];
        plic.threshold[i] = blob->plic_threshold[i];
    }
    uart.ier = blob->uart_ier;
    uart.fcr = blob->uart_fcr;
    uart.lcr = blob->uart_lcr;
    uart.mcr = blob->uart_mcr;
    uart.scr = blob->uart_scr;
    uart.dll = blob->uart_dll;
    uart.dlm = blob->uart_dlm;
    memcpy(uart.fifo, blob->uart_fifo, UART16550_FIFO_SIZE);
    uart.fifo_read = blob->uart_fifo_read;
    uart.fifo_write = blob->uart_fifo_write;
    uart.fifo_count = blob->uart_fifo_count;
    uart.overrun = blob->uart_overrun;
    uart.thre_pending = blob->uart_thre_pending;
    unpack_mmio(&virtio_blk.mmio, &blob->virtio[0]);
    virtio_blk.capacity = blob->virtio_blk_capacity;
    unpack_mmio(&virtio_console.mmio, &blob->virtio[1]);
    memcpy(virtio_console.rx_fifo, blob->virtio_console_rx_fifo, VIRTIO_CONSOLE_RX_FIFO_SIZE);
    virtio_console.rx_read = blob->virtio_console_rx_read;
    virtio_console.rx_write = blob->virtio_console_rx_write;
    virtio_console.rx_count = blob->virtio_console_rx_count;
    virtio_console.rx_overrun = blob->virtio_console_rx_overrun;
    unpack_mmio(&virtio_net.mmio, &blob->virtio[2]);
    unpack_mmio(&virtio_rng.mmio, &blob->virtio[3]);
    goldfish_rtc.base_ns = blob->rtc_base_ns;
    goldfish_rtc.anchor_ns = vpi_rtc_monotonic_ns();
    goldfish_rtc.latch_high = blob->rtc_latch_high;
    goldfish_rtc.alarm_low = blob->rtc_alarm_low;
    goldfish_rtc.alarm_high = blob->rtc_alarm_high;
    goldfish_rtc.irq_enabled = blob->rtc_irq_enabled;
    goldfish_rtc.alarm_status = blob->rtc_alarm_status;
}

/**
 * Handles a host snapshot request in the AVR main loop.
 */
void snapshot_poll(void) {
    uint8_t request = vpi_snapshot_request();
    if (request == 0) {
        return;
    }
    if (request == 1) {
        rve_snapshot_t blob;
        snapshot_pack(&blob);
        uint32_t offset = RVE_SNAPSHOT_META_ADDRESS - RVE_SNAPSHOT_RAM_ORIGIN;
        uint32_t magic = SNAPSHOT_MAGIC;
        vpi_store(offset, &magic, sizeof(magic));
        uint32_t length = sizeof(blob);
        vpi_store(offset + 4u, &length, sizeof(length));
        vpi_store(offset + 8u, &blob, sizeof(blob));
        vpi_state_saved();
    } else if (request == 2) {
        uint32_t offset = RVE_SNAPSHOT_META_ADDRESS - RVE_SNAPSHOT_RAM_ORIGIN;
        uint32_t magic = 0;
        uint32_t length = 0;
        vpi_load(offset, &magic, sizeof(magic));
        vpi_load(offset + 4u, &length, sizeof(length));
        if (magic == SNAPSHOT_MAGIC && length == sizeof(rve_snapshot_t)) {
            rve_snapshot_t blob;
            vpi_load(offset + 8u, &blob, sizeof(blob));
            if (blob.magic == RVE_SNAPSHOT_MAGIC && blob.version == RVE_SNAPSHOT_VERSION) {
                snapshot_unpack(&blob);
            }
        }
    }
    vpi_snapshot_acknowledge();
}

#endif
