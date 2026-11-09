#include "pw_internal.h"

#define TICKS_PER_SEC     32768u
#define RTC_QUARTER_TICKS (TICKS_PER_SEC / 4)
#define TIMER_B_TICKS     (TICKS_PER_SEC / 16)

#define ROM_FREQTAB 0xBB4A

#define RTC_QUARTER 0x01
#define RTC_HALF    0x02
#define RTC_SEC     0x04
#define RTC_MIN     0x08
#define RTC_HOUR    0x10

static uint8_t bcd_inc(uint8_t v, uint8_t limit, int *wrapped) {
    v = (v & 0xF) == 9 ? (uint8_t)((v & 0xF0) + 0x10) : (uint8_t)(v + 1);
    *wrapped = v >= limit;
    return *wrapped ? 0 : v;
}

static void beep_update(pw_state_t *s) {
    pw_hw_t *hw = &s->hw;
    uint32_t freq = 0;
    uint8_t vol = 0;
    if (hw->tw_clock && (hw->tmrw & 0x80) && (hw->tmrw & 3) == 3 && hw->gra) {
        int b = hw->grb != hw->gra;
        int c = hw->grc != hw->gra;
        vol = (uint8_t)(b + c);
        if (vol) {
            freq = TICKS_PER_SEC / hw->gra;
        }
    }
    if (freq != hw->beep_freq || vol != hw->beep_vol) {
        hw->beep_freq = freq;
        hw->beep_vol = vol;
        if (s->hal && s->hal->beep) {
            s->hal->beep(s->hal->user, freq, vol);
        }
    }
}

static uint32_t tw_period(const pw_hw_t *hw) {
    return hw->gra ? hw->gra : 1;
}

static void tw_restart(pw_state_t *s) {
    s->hw.tw_start = s->hw.now;
}

void hw_resync_beeper(pw_state_t *s) {
    s->hw.beep_freq = 0xFFFFFFFF;
    beep_update(s);
}

void hw_reset(pw_state_t *s) {
    pw_hw_t *hw = &s->hw;
    hw->rtc_next = hw->now + RTC_QUARTER_TICKS;
    hw->beep_freq = 0;
    hw->beep_vol = 0;
}

void hw_rtc_set(pw_state_t *s, uint8_t sec, uint8_t min, uint8_t hour) {
    pw_hw_t *hw = &s->hw;
    hw->rtc_sec = sec;
    hw->rtc_min = min;
    hw->rtc_hour = hour;
    hw->rtc_quarter = 0;
    hw->rtc_next = hw->now + RTC_QUARTER_TICKS;
    hw->rtccr2 = RTC_SEC | RTC_MIN | RTC_HOUR;
    hw->rtcflg = 0;
}

void hw_rtc_quarter_irq(pw_state_t *s, int on) {
    if (on) {
        s->hw.rtccr2 |= RTC_QUARTER;
    } else {
        s->hw.rtccr2 &= ~RTC_QUARTER;
    }
}

void hw_rtc_read(pw_state_t *s, uint8_t *sec, uint8_t *min, uint8_t *hour) {
    *sec = s->hw.rtc_sec;
    *min = s->hw.rtc_min;
    *hour = s->hw.rtc_hour;
}

static void rtc_step(pw_state_t *s) {
    pw_hw_t *hw = &s->hw;
    hw->rtc_quarter = (hw->rtc_quarter + 1) & 3;
    uint8_t flags = RTC_QUARTER;
    if (!(hw->rtc_quarter & 1)) {
        flags |= RTC_HALF;
    }
    if (!hw->rtc_quarter) {
        int wrapped;
        flags |= RTC_SEC;
        hw->rtc_sec = bcd_inc(hw->rtc_sec, 0x60, &wrapped);
        if (wrapped) {
            flags |= RTC_MIN;
            hw->rtc_min = bcd_inc(hw->rtc_min, 0x60, &wrapped);
            if (wrapped) {
                flags |= RTC_HOUR;
                hw->rtc_hour = bcd_inc(hw->rtc_hour, 0x24, &wrapped);
            }
        }
    }
    hw->rtcflg |= flags & hw->rtccr2;
    hw->rtc_next += RTC_QUARTER_TICKS;
}

static void isr_rtc_quarter(pw_state_t *s) {
    wr8(s, RAM_common_bit_flags, rd8(s, RAM_common_bit_flags) | 1);
}

static void isr_rtc_sec(pw_state_t *s) {
    wr8(s, RAM_currentSecond, s->hw.rtc_sec);
    wr32(s, RAM_RamCache_lastSyncTime, rd32(s, RAM_RamCache_lastSyncTime) + 1);
    uint16_t t = rd16(s, RAM_word_FFF7A2) + 1;
    wr16(s, RAM_word_FFF7A2, t > 0xE10 ? 0xE10 : t);
    if (rd8(s, RAM_byte_FFF7AF)) {
        wr8(s, RAM_byte_FFF7AF, rd8(s, RAM_byte_FFF7AF) - 1);
    }
    if (rd8(s, RAM_byte_FFF7B0)) {
        wr8(s, RAM_byte_FFF7B0, rd8(s, RAM_byte_FFF7B0) - 1);
    }
}

static void isr_rtc_min(pw_state_t *s) {
    wr8(s, RAM_currentMinute, s->hw.rtc_min);
    wr8(s, RAM_byte_FFF7A7, rd8(s, RAM_byte_FFF7A7) | 1);
}

static void isr_rtc_hour(pw_state_t *s) {
    wr8(s, RAM_currentHour, s->hw.rtc_hour);
    wr8(s, RAM_byte_FFF7A7, rd8(s, RAM_byte_FFF7A7) | 2);
}

void set_timer_b_and_enable_irq(pw_state_t *s) {
    s->hw.tb_on = 1;
    s->hw.tb_next = s->hw.now + TIMER_B_TICKS;
}

void hw_timer_b(pw_state_t *s, int on) {
    if (on && !s->hw.tb_on) {
        s->hw.tb_next = s->hw.now + TIMER_B_TICKS;
    }
    s->hw.tb_on = (uint8_t)on;
}

static void isr_irq0(pw_state_t *s) {
    wr8(s, RAM_common_bit_flags, rd8(s, RAM_common_bit_flags) | 8);
    wr8(s, RAM_byte_FFF7BB, 1);
    hw_timer_b(s, 1);
}

void sub_37D6(pw_state_t *s) {
    pw_hw_t *hw = &s->hw;
    hw->tw_clock = 1;
    hw->tw_irq = 1;
    hw->tmrw = 0x80;
    tw_restart(s);
    beep_update(s);
}

void sub_3810(pw_state_t *s) {
    pw_hw_t *hw = &s->hw;
    hw->tw_irq = 0;
    hw->tmrw = 0;
    hw->tw_clock = 0;
    beep_update(s);
}

void configureSomeIoAndTimerW(pw_state_t *s) {
    wr8(s, RAM_byte_FFF7CC, 0x78);
    wr8(s, RAM_likelyVolume, 0);
    s->hw.gra = s->hw.grb = s->hw.grc = 0;
    s->hw.tw_clock = 0;
    wr16(s, RAM_word_FFF7C4, 0);
    beep_update(s);
}

void setBeeperFreq(pw_state_t *s, uint8_t period) {
    pw_hw_t *hw = &s->hw;
    switch (rd8(s, RAM_likelyVolume)) {
    case 0: hw->gra = hw->grb = hw->grc = period; break;
    case 1:
        hw->gra = period;
        hw->grb = period >> 1;
        hw->grc = period;
        break;
    case 2:
        hw->gra = period;
        hw->grb = hw->grc = period >> 1;
        break;
    }
    tw_restart(s);
    beep_update(s);
}

static uint16_t note_length(pw_state_t *s, uint8_t len, int gap, uint8_t note) {
    uint32_t d = (uint32_t)len * 0x14000 / rd8(s, RAM_byte_FFF7CC);
    if (gap) {
        d = (d & 0xFFFF0000) | ((d - 0x140) & 0xFFFF);
    }
    return (uint16_t)(d / rom_rd8(ROM_FREQTAB + note));
}

void timer_w_irq(pw_state_t *s) {
    pw_hw_t *hw = &s->hw;
    uint16_t p = rd16(s, RAM_word_FFF7C4);
    if (!p) {
        return;
    }
    uint16_t left = rd16(s, RAM_word_FFF7C8);
    if (left) {
        left--;
        wr16(s, RAM_word_FFF7C8, left);
        if (left == 1 && (mem_rd8(s, p + 1) & 0x7F) == 0x7F) {
            hw->tmrw = 0x80;
            beep_update(s);
        }
        if (left) {
            return;
        }
    }
    uint16_t gap = rd16(s, RAM_word_FFF7CA);
    if (gap) {
        hw->gra = hw->grb = hw->grc = 0x140;
        tw_restart(s);
        beep_update(s);
        wr16(s, RAM_word_FFF7CA, gap - 1);
        return;
    }

    uint8_t note = mem_rd8(s, p + 1) & 0x7F;
    if (note == 0x7F) {
        wr16(s, RAM_word_FFF7C4, 0);
        return;
    }
    if (note == 0x7B) {
        wr8(s, RAM_byte_FFF7CC, mem_rd8(s, p));
        p += 2;
        wr16(s, RAM_word_FFF7C4, p);
    }
    note = mem_rd8(s, p + 1) & 0x7F;
    if (note == 0x7E) {
        wr16(s, RAM_word_FFF7C4, RAM_ACCEL_SAMPLES_X);
        return;
    }
    uint8_t len = mem_rd8(s, p);
    if (note == 0x7D) {
        wr16(s, RAM_word_FFF7C8, note_length(s, len, 0, note));
        setBeeperFreq(s, 0);
    } else {
        if (mem_rd8(s, p + 1) & 0x80) {
            wr16(s, RAM_word_FFF7C8, note_length(s, len, 0, note));
            wr16(s, RAM_word_FFF7CA, 0);
        } else {
            wr16(s, RAM_word_FFF7C8, note_length(s, len, 1, note));
            wr16(s, RAM_word_FFF7CA, 1);
        }
        if (p == RAM_ACCEL_SAMPLES_X || !(mem_rd8(s, p - 1) & 0x80)) {
            hw->tmrw = 0x83;
            setBeeperFreq(s, rom_rd8(ROM_FREQTAB + note));
        }
    }
    wr16(s, RAM_word_FFF7C4, p + 2);
}

void hw_ir_start(pw_state_t *s) {
    pw_hw_t *hw = &s->hw;
    hw->tw_clock = 1;
    hw->tw_irq = 0;
    hw->tmrw |= 0x80;
    hw->ir_base = hw->now;
    beep_update(s);
}

void hw_ir_stop(pw_state_t *s) {
    s->hw.tmrw &= ~0x80;
    s->hw.tw_clock = 0;
    beep_update(s);
}

uint16_t hw_ir_tcnt(pw_state_t *s) {
    return (uint16_t)(s->hw.now - s->hw.ir_base);
}

void hw_mask_interrupts(pw_state_t *s, int masked) {
    s->hw.masked = (uint8_t)masked;
}

static uint64_t tw_next(const pw_hw_t *hw) {
    uint32_t period = tw_period(hw);
    uint64_t elapsed = hw->now - hw->tw_start;
    return hw->tw_start + (elapsed / period + 1) * period;
}

static void service_rtc(pw_state_t *s) {
    uint8_t f = s->hw.rtcflg;
    s->hw.rtcflg = 0;
    if (f & RTC_QUARTER)
        isr_rtc_quarter(s);
    if (f & RTC_SEC)
        isr_rtc_sec(s);
    if (f & RTC_MIN)
        isr_rtc_min(s);
    if (f & RTC_HOUR)
        isr_rtc_hour(s);
}

int hw_wait(pw_state_t *s, uint64_t limit, int timer_b_wakes) {
    pw_hw_t *hw = &s->hw;
    if (!hw->masked) {
        if (hw->irq0_flag) {
            hw->irq0_flag = 0;
            isr_irq0(s);
            return 1;
        }
        if (hw->rtcflg) {
            service_rtc(s);
            return 1;
        }
    }
    for (;;) {
        uint64_t t = hw->rtc_next;
        int tw = hw->tw_clock && hw->tw_irq;
        uint64_t twt = tw ? tw_next(hw) : UINT64_MAX;
        if (twt < t) {
            t = twt;
        }
        if (hw->tb_on && hw->tb_next < t) {
            t = hw->tb_next;
        }
        if (t > limit) {
            if (limit > hw->now) {
                hw->now = limit;
            }
            return 0;
        }
        hw->now = t;

        int woke = 0;
        if (hw->tb_on && hw->tb_next == t) {
            hw->tb_next += TIMER_B_TICKS;
            woke |= timer_b_wakes;
        }
        if (hw->rtc_next == t) {
            rtc_step(s);
            if (hw->rtcflg && !hw->masked) {
                service_rtc(s);
                woke = 1;
            }
        }
        if (tw && twt == t) {
            hw->tw_start = t;
            if (!hw->masked) {
                timer_w_irq(s);
                woke = 1;
            }
        }
        if (woke && !hw->masked) {
            return 1;
        }
    }
}

uint8_t hw_buttons(pw_state_t *s) {
    uint8_t b = s->hal && s->hal->buttons ? s->hal->buttons(s->hal->user) : 0;
    if ((b & PW_BUTTON_ENTER) && !(s->hw.keys_last & PW_BUTTON_ENTER)) {
        s->hw.irq0_flag = 1;
    }
    s->hw.keys_last = b;
    return b;
}

uint64_t hw_ms_to_ticks(uint32_t ms) {
    return (uint64_t)ms * TICKS_PER_SEC / 1000;
}
