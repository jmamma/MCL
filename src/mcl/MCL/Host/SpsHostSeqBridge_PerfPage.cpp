#if !defined(__AVR__)

#include "Host/SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"

#include "GUI/Pages/CommonPages.h"
#include "GUI/Pages/Performance/PerfEncoder.h"
#include "PerfData.h"

#include <string.h>

using namespace spsseq;
using namespace sps_host_seq_internal;

namespace {

uint8_t clamp7(uint8_t value) {
    return value > 127 ? 127 : value;
}

PerfEncoder* perfPageEncoder(uint8_t control) {
    switch (control) {
    case 0:
        return perf_page.perf_encoders[0] ? perf_page.perf_encoders[0]
                                          : &perf_param1;
    case 1:
        return perf_page.perf_encoders[1] ? perf_page.perf_encoders[1]
                                          : &perf_param2;
    case 2:
        return perf_page.perf_encoders[2] ? perf_page.perf_encoders[2]
                                          : &perf_param3;
    case 3:
        return perf_page.perf_encoders[3] ? perf_page.perf_encoders[3]
                                          : &perf_param4;
    default:
        return nullptr;
    }
}

uint8_t sceneCount(const PerfScene& scene) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < NUM_PERF_PARAMS; i++) {
        if (scene.params[i].dest != 0)
            count++;
    }
    return count;
}

void recomputeSceneCount(PerfScene& scene) {
    scene.count = sceneCount(scene);
}

uint16_t activeSceneMask() {
    uint16_t mask = 0;
    for (uint8_t scene = 0; scene < NUM_SCENES; scene++) {
        if (sceneCount(PerfData::scenes[scene]) > 0)
            mask |= (uint16_t)(1u << scene);
    }
    return mask;
}

void putName(uint8_t* dst, const char* name) {
    for (uint8_t i = 0; i < kPerfPageNameBytes; i++)
        dst[i] = ' ';
    if (!name)
        return;
    for (uint8_t i = 0; i < kPerfPageNameBytes && name[i] != '\0'; i++)
        dst[i] = (uint8_t)name[i];
}

} // namespace

void SpsHostSeqBridge::sendPerfPageState(uint8_t tag) {
    uint8_t body[kPerfPageStateWireBytes];
    uint16_t off = 0;
    body[off++] = perf_page.perf_id < kPerfPageControls ? perf_page.perf_id
                                                        : 0;
    body[off++] = perf_page.page_mode <= kPerfPageParams ? perf_page.page_mode
                                                         : 0;
    body[off++] = perf_page.learn;
    putU16le(body + off, activeSceneMask());
    off = (uint16_t)(off + 2);

    for (uint8_t control = 0; control < kPerfPageControls; control++) {
        PerfEncoder* e = perfPageEncoder(control);
        if (e == nullptr) {
            for (uint8_t i = 0; i < kPerfPageControlWireBytes; i++)
                body[off++] = 0;
            continue;
        }
        body[off++] = clamp7(e->cur);
        body[off++] = clamp7(e->perf_data.src);
        body[off++] = clamp7(e->perf_data.param);
        body[off++] = clamp7(e->perf_data.min);
        body[off++] = e->active_scene_a < NUM_SCENES ? e->active_scene_a
                                                     : (uint8_t)0xFF;
        body[off++] = e->active_scene_b < NUM_SCENES ? e->active_scene_b
                                                     : (uint8_t)0xFF;
        putName(body + off, e->name);
        off = (uint16_t)(off + kPerfPageNameBytes);
    }

    for (uint8_t scene = 0; scene < kPerfPageScenes; scene++) {
        PerfScene& s = PerfData::scenes[scene];
        recomputeSceneCount(s);
        body[off++] = s.count;
        for (uint8_t paramIndex = 0; paramIndex < kPerfPageParams;
             paramIndex++) {
            PerfParam& p = s.params[paramIndex];
            body[off++] = clamp7(p.dest);
            body[off++] = clamp7(p.param);
            body[off++] = p.val <= 127 ? p.val : (uint8_t)0xFF;
        }
    }

    sendFrame(CMD_PERF_PAGE_STATE, tag, body, off);
}

bool SpsHostSeqBridge::applyPerfPageSetControl(const uint8_t* b,
                                                uint16_t n) {
    if (n < 5 || b[0] >= kPerfPageControls)
        return false;
    PerfEncoder* e = perfPageEncoder(b[0]);
    if (e == nullptr)
        return false;
    e->perf_data.update_src(clamp7(b[1]), clamp7(b[2]), clamp7(b[3]));
    e->cur = clamp7(b[4]);
    e->old = e->cur;
    e->resend = true;
    perf_page.perf_id = b[0];
    perf_page.config_encoders();
    return true;
}

bool SpsHostSeqBridge::applyPerfPageSetActiveScene(const uint8_t* b,
                                                   uint16_t n) {
    if (n < 3 || b[0] >= kPerfPageControls || b[1] > 1)
        return false;
    PerfEncoder* e = perfPageEncoder(b[0]);
    if (e == nullptr)
        return false;
    uint8_t scene = b[2] < NUM_SCENES ? b[2] : (uint8_t)0xFF;
    if (b[1] == 0)
        e->active_scene_a = scene;
    else
        e->active_scene_b = scene;
    perf_page.perf_id = b[0];
    return true;
}

bool SpsHostSeqBridge::applyPerfPageSetSceneParam(const uint8_t* b,
                                                  uint16_t n) {
    if (n < 5 || b[0] >= kPerfPageScenes || b[1] >= kPerfPageParams)
        return false;
    PerfScene& scene = PerfData::scenes[b[0]];
    PerfParam& p = scene.params[b[1]];
    if (b[2] == 0 || b[4] > 127) {
        p.dest = 0;
        p.param = 0;
        p.val = 255;
    } else {
        p.dest = clamp7(b[2]);
        p.param = clamp7(b[3]);
        p.val = clamp7(b[4]);
    }
    recomputeSceneCount(scene);
    perf_page.config_encoders();
    return true;
}

bool SpsHostSeqBridge::applyPerfPageSceneAction(const uint8_t* b,
                                                uint16_t n) {
    if (n < 3 || b[1] >= kPerfPageControls || b[2] >= kPerfPageScenes)
        return false;
    PerfEncoder* e = perfPageEncoder(b[1]);
    if (e == nullptr)
        return false;
    switch (b[0]) {
    case PERF_PAGE_SCENE_CLEAR:
        e->perf_data.clear_scene(b[2]);
        break;
    case PERF_PAGE_SCENE_SEND:
        e->send_params(0, &PerfData::scenes[b[2]], nullptr);
        break;
    case PERF_PAGE_SCENE_AUTOFILL:
        e->perf_data.scene_autofill(b[2]);
        break;
    default:
        return false;
    }
    perf_page.perf_id = b[1];
    perf_page.config_encoders();
    return true;
}

bool SpsHostSeqBridge::applyPerfPageSetView(const uint8_t* b, uint16_t n) {
    if (n < 3 || b[0] >= kPerfPageControls)
        return false;
    perf_page.perf_id = b[0];
    perf_page.page_mode = b[1] <= kPerfPageParams ? b[1] : PERF_DESTINATION;
    perf_page.config_encoders();
    return true;
}

#endif  // !defined(__AVR__)
