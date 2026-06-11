#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Host/SpsHostArrBridge.h"
#include "SpsHostArrBridge_Internal.h"

using namespace spsarr;
using namespace sps_host_arr_internal;

void SpsHostArrBridge::onReqProjectList(uint8_t tag, const uint8_t* b,
                                        uint16_t n) {
    if (n < 4) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint16_t offset = spsArrGetU16(b + 0);
    uint8_t maxEntries = b[2];
    if (maxEntries == 0 ||
        maxEntries > (uint8_t)spsarr::kMaxProjectEntriesPerFrame) {
        maxEntries = (uint8_t)spsarr::kMaxProjectEntriesPerFrame;
    }

    uint16_t off = 3;
    char cwd[PRJ_PATH_LEN];
    if (!readProjectBodyString(b, n, off, cwd, sizeof(cwd), true)) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }

    uint8_t body[spsarr::kProjectListHeaderBytes +
                 spsarr::kMaxProjectEntriesPerFrame *
                     spsarr::kProjectEntryRecordBytes] = {};
    char rooted[128];
    if (!buildProjectRootedPath(cwd, rooted, sizeof(rooted)) ||
        !SD.chdir(rooted)) {
        uint16_t len = fillProjectListError(body, cwd);
        sendFrame(CMD_PROJECT_LIST, tag, body, len);
        return;
    }

    File dir;
    if (!dir.open(rooted, O_READ) || !dir.isDirectory()) {
        dir.close();
        uint16_t len = fillProjectListError(body, cwd);
        sendFrame(CMD_PROJECT_LIST, tag, body, len);
        return;
    }
    dir.rewind();

    char currentChild[PRJ_NAME_LEN + 1];
    bool haveCurrentChild =
        currentProjectChildForCwd(cwd, currentChild, sizeof(currentChild));
    uint16_t currentIndex = 0xFFFF;
    uint16_t total = 0;
    uint8_t count = 0;

    appendProjectEntry(body, total, offset, maxEntries, count,
                       PROJECT_ENTRY_NEW, 0, "[ NEW PROJECT ]");
    total++;
    if (cwd[0] != '\0') {
        appendProjectEntry(body, total, offset, maxEntries, count,
                           PROJECT_ENTRY_PARENT, 0, "..");
        total++;
    }

    File entryFile;
    char entry[64];
    char entryPath[PRJ_PATH_LEN];
    while (entryFile.openNext(&dir, O_READ)) {
        entryFile.getName(entry, sizeof(entry));
        bool isDir = entryFile.isDirectory();
        entryFile.close();

        if (!isDir || entry[0] == '\0' || entry[0] == '.')
            continue;
        if (!simpleProjectName(entry))
            continue;
        uint8_t type = isProjectDirNameAtCwd(entry)
                           ? PROJECT_ENTRY_PROJECT
                           : PROJECT_ENTRY_DIR;
        if (!joinProjectRelPath(cwd, entry, entryPath, sizeof(entryPath)))
            continue;
        uint8_t flags = projectEntryFlags(type, entryPath);
        if (haveCurrentChild && strcmp(entry, currentChild) == 0) {
            currentIndex = total;
            flags |= PROJECT_ENTRY_CURRENT;
        }
        appendProjectEntry(body, total, offset, maxEntries, count, type,
                           flags, entry);
        total++;
    }
    entryFile.close();
    dir.close();

    uint8_t flags = 0;
    if ((uint16_t)(offset + count) < total)
        flags |= PROJECT_LIST_MORE;
#ifdef MCL_HAS_PROJECT_BACKUP
    flags |= PROJECT_LIST_BACKUP;
#endif
#ifdef MCL_HAS_FILE_MOVE
    flags |= PROJECT_LIST_MOVE;
#endif
    if (proj.project_loaded)
        flags |= PROJECT_LIST_PROJECT_LOADED;

    fillProjectListHeader(body, flags, offset, count, total, currentIndex,
                          cwd);
    uint16_t bodyLen =
        (uint16_t)(spsarr::kProjectListHeaderBytes +
                   count * spsarr::kProjectEntryRecordBytes);
    sendFrame(CMD_PROJECT_LIST, tag, body, bodyLen);
}

void SpsHostArrBridge::onReqProjectVersions(uint8_t tag, const uint8_t* b,
                                            uint16_t n) {
#ifndef MCL_HAS_PROJECT_BACKUP
    (void)b;
    (void)n;
    sendErr(tag, ERR_UNSUPPORTED, 0);
#else
    uint16_t off = 0;
    char project[PRJ_PATH_LEN];
    if (!readProjectBodyString(b, n, off, project, sizeof(project), false) ||
        !projectRelPathIsProject(project)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t activePair = 0;
    bool haveActive = proj.read_active_grid_pair(project, &activePair);
    char base[PRJ_NAME_LEN + 1];
    if (!chdirProjectParent(project, base, sizeof(base)) ||
        !SD.chdir(base)) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }

    uint8_t body[spsarr::kProjectVersionHeaderBytes +
                 128 * spsarr::kProjectVersionRecordBytes] = {};
    uint16_t total = 0;
    uint8_t count = 0;
    for (uint8_t pair = 0; pair < 128; ++pair) {
        if (!proj.project_pair_exists(pair, base))
            continue;
        uint16_t recordOff =
            (uint16_t)(spsarr::kProjectVersionHeaderBytes +
                       count * spsarr::kProjectVersionRecordBytes);
        body[recordOff] = pair;
        body[recordOff + 1] =
            (uint8_t)((haveActive && pair == activePair
                           ? PROJECT_VERSION_ACTIVE
                           : 0) |
                      (pair > 0 && (!haveActive || pair != activePair)
                           ? PROJECT_VERSION_CAN_DELETE
                           : 0));
        count++;
        total++;
    }

    body[0] = 0;
    body[1] = haveActive ? activePair : 0;
    spsArrPutU16(body + 2, total);
    body[4] = count;
    body[5] = (uint8_t)strlen(project);
    body[6] = 0;
    body[7] = 0;
    memset(body + 8, 0, spsarr::kProjectPathBytes);
    memcpy(body + 8, project, strlen(project));
    sendFrame(CMD_PROJECT_VERSIONS, tag, body,
              (uint16_t)(spsarr::kProjectVersionHeaderBytes +
                         count * spsarr::kProjectVersionRecordBytes));
#endif
}

void SpsHostArrBridge::onProjectOp(uint8_t tag, const uint8_t* b,
                                   uint16_t n) {
    if (n < 3) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t op = b[0];
    uint16_t off = 1;
    char path[PRJ_PATH_LEN];
    char arg[PRJ_PATH_LEN];
    if (!readProjectBodyString(b, n, off, path, sizeof(path), true) ||
        !readProjectBodyString(b, n, off, arg, sizeof(arg), true)) {
        sendErr(tag, ERR_RANGE, op);
        return;
    }

    bool ok = false;
    bool loadedProject = false;

    switch (op) {
        case PROJECT_OP_LOAD:
            ok = path[0] != '\0' && projectRelPathIsProject(path) &&
                 proj.load_project(path);
            if (ok) {
                grid_page.reload_slot_models = false;
                loadedProject = true;
            }
            break;

        case PROJECT_OP_NEW_PROJECT: {
            char newPath[PRJ_PATH_LEN];
            ok = simpleProjectName(arg) &&
                 joinProjectRelPath(path, arg, newPath, sizeof(newPath)) &&
                 proj.new_project(newPath) && proj.load_project(newPath);
            if (ok) {
                grid_page.reload_slot_models = false;
                loadedProject = true;
            }
            break;
        }

        case PROJECT_OP_NEW_FOLDER: {
            char newPath[PRJ_PATH_LEN];
            ok = simpleProjectName(arg) &&
                 joinProjectRelPath(path, arg, newPath, sizeof(newPath));
            if (ok) {
                proj.chdir_projects();
                ok = !SD.exists(newPath) && SD.mkdir(newPath, true);
            }
            break;
        }

        case PROJECT_OP_DELETE:
            ok = path[0] != '\0' && !currentProjectUnder(path);
            if (ok) {
                proj.chdir_projects();
                ok = mcl_sd.remove_dir(path);
            }
            break;

        case PROJECT_OP_RENAME: {
            ok = path[0] != '\0' && arg[0] != '\0' &&
                 sameProjectParent(path, arg);
            if (!ok)
                break;

            char baseFrom[PRJ_NAME_LEN + 1];
            char baseTo[PRJ_NAME_LEN + 1];
            char parent[PRJ_PATH_LEN];
            if (!splitProjectRelPath(path, parent, sizeof(parent), baseFrom,
                                     sizeof(baseFrom)) ||
                !splitProjectRelPath(arg, parent, sizeof(parent), baseTo,
                                     sizeof(baseTo)) ||
                !simpleProjectName(baseTo)) {
                ok = false;
                break;
            }

            char parentRoot[128];
            ok = buildProjectRootedPath(parent, parentRoot,
                                        sizeof(parentRoot)) &&
                 SD.chdir(parentRoot);
            if (!ok)
                break;

            bool isProject = isProjectDirNameAtCwd(baseFrom);
            bool reloadCurrent =
                isProject && proj.project_loaded &&
                strcmp(mcl_cfg.project, path) == 0;
            if (!isProject && currentProjectUnder(path)) {
                ok = false;
                break;
            }
            if (isProject) {
                ok = SD.chdir(baseFrom) &&
                     proj.rename_project_files(baseFrom, baseTo);
                SD.chdir(parentRoot);
                ok = ok && SD.rename(baseFrom, baseTo);
                if (ok && reloadCurrent) {
                    ok = proj.load_project(arg);
                    loadedProject = ok;
                }
            } else {
                ok = SD.rename(baseFrom, baseTo);
            }
            break;
        }

        case PROJECT_OP_COPY:
            ok = path[0] != '\0' && arg[0] != '\0';
            if (ok) {
                bool isProject = projectRelPathIsProject(path);
                if (isProject) {
                    ok = proj.copy_project(path, arg);
                } else {
                    proj.chdir_projects();
                    ok = mcl_sd.copy_dir(path, arg, 0, 64, 64);
                }
            }
            break;

        case PROJECT_OP_MOVE:
#ifndef MCL_HAS_FILE_MOVE
            sendErr(tag, ERR_UNSUPPORTED, op);
            return;
#else
            ok = path[0] != '\0' && arg[0] != '\0' &&
                 strcmp(path, arg) != 0 &&
                 !pathStartsWithProjectDir(arg, path) &&
                 !currentProjectUnder(path);
            if (ok) {
                bool isProject = projectRelPathIsProject(path);
                if (isProject) {
                    ok = proj.move_project(path, arg);
                } else {
                    proj.chdir_projects();
                    ok = SD.rename(path, arg);
                }
            }
            break;
#endif

        default:
            sendErr(tag, ERR_UNSUPPORTED, op);
            return;
    }

    if (!ok) {
        sendErr(tag, ERR_RANGE, op);
        return;
    }

    uint8_t ack[2] = {CMD_PROJECT_OP, op};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    uint8_t dirty = DIRTY_PROJECTS;
    if (loadedProject) {
        dirty |= (uint8_t)(DIRTY_CELLS | DIRTY_ACTIVE |
                           DIRTY_ARRANGEMENT);
    }
    notifyDirty(0xFF, dirty);
}

void SpsHostArrBridge::onProjectVersionOp(uint8_t tag, const uint8_t* b,
                                          uint16_t n) {
#ifndef MCL_HAS_PROJECT_BACKUP
    (void)b;
    (void)n;
    sendErr(tag, ERR_UNSUPPORTED, 0);
#else
    if (n < 3) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t op = b[0];
    uint8_t pair = b[1];
    uint16_t off = 2;
    char project[PRJ_PATH_LEN];
    if (!readProjectBodyString(b, n, off, project, sizeof(project), false) ||
        !projectRelPathIsProject(project)) {
        sendErr(tag, ERR_RANGE, op);
        return;
    }

    bool ok = false;
    bool loadedProject = false;
    switch (op) {
        case PROJECT_VERSION_CREATE_BACKUP: {
            uint8_t createdPair = 0;
            ok = proj.create_backup(project, &createdPair) &&
                 proj.load_project_version(project, createdPair);
            loadedProject = ok;
            break;
        }
        case PROJECT_VERSION_LOAD:
            ok = pair < 128 && proj.load_project_version(project, pair);
            loadedProject = ok;
            break;
        case PROJECT_VERSION_DELETE:
            ok = pair > 0 && proj.delete_backup(project, pair);
            break;
        default:
            sendErr(tag, ERR_UNSUPPORTED, op);
            return;
    }

    if (!ok) {
        sendErr(tag, ERR_RANGE, op);
        return;
    }

    uint8_t ack[2] = {CMD_PROJECT_VERSION_OP, op};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    uint8_t dirty = DIRTY_PROJECTS;
    if (loadedProject) {
        dirty |= (uint8_t)(DIRTY_CELLS | DIRTY_ACTIVE |
                           DIRTY_ARRANGEMENT);
    }
    notifyDirty(0xFF, dirty);
#endif
}

#endif  // MCL_FEATURE_HOST_ARRANGER
