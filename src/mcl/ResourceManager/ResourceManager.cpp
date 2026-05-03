#include "ResourceManager.h"
#include "MCLMenus.h"

ResourceManager::ResourceManager() { }

void ResourceManager::Clear() {
    DEBUG_PRINTLN("resource clear");
	m_bufsize = 0;
}

byte* ResourceManager::__use_resource(const void* pgm) {
    byte* pos = m_buffer + m_bufsize;
	uint16_t sz = unpack((byte*)pgm, pos);
	m_bufsize += sz;
    DEBUG_PRINTLN("resource buf size");
    DEBUG_PRINTLN(m_bufsize);
    return pos;
}

byte* ResourceManager::Allocate(size_t sz) {
  byte* pos = m_buffer + m_bufsize;
  m_bufsize += sz;
  DEBUG_PRINTLN("allocate");
  DEBUG_PRINTLN(m_bufsize);
  return pos;
}

void ResourceManager::Free(size_t sz) {
  m_bufsize -= sz;
}

// XXX 4KB buf on stack is too heavy
// consider writing to SD card
// SWAP partition!!
void ResourceManager::Save(uint8_t *buf, size_t *sz) {
    DEBUG_PRINTLN("resource SAVE");
	memcpy(buf, m_buffer, m_bufsize);
	*sz = m_bufsize;
}

void ResourceManager::Restore(uint8_t *buf, size_t sz) {
	memcpy(m_buffer, buf, sz);
	m_bufsize = sz;
}

void ResourceManager::restore_page_entry_deps() {
  PageSelectEntry* entry = R.page_entries->Entries;
  struct __T_icons_page *icons = R.icons_page;

  (entry++)->IconData = icons->icon_grid;
  (entry++)->IconData = icons->icon_mixer;
  (entry++)->IconData = icons->icon_perf;
  (entry++)->IconData = icons->icon_route;
  (entry++)->IconData = icons->icon_step;
  (entry++)->IconData = icons->icon_lfo;
  (entry++)->IconData = icons->icon_pianoroll;
  (entry++)->IconData = icons->icon_chroma;

#ifdef SOUND_PAGE
  (entry++)->IconData = icons->icon_sample;
#endif
#ifdef WAV_DESIGNER
  (entry++)->IconData = icons->icon_wavd;
#endif
#ifdef LOUDNESS_PAGE
  (entry++)->IconData = icons->icon_loudness;
#endif

  (entry++)->IconData = icons->icon_rhytmecho;
  (entry++)->IconData = icons->icon_gatebox;
  (entry++)->IconData = icons->icon_ram1;
  entry->IconData = icons->icon_ram2;
}

size_t ResourceManager::Size() {
  return m_bufsize;
}

void ResourceManager::restore_menu_layout_deps() {
  aux_config_page.set_layout(R.menu_layouts->auxconfig_menu_layout);
  system_page.set_layout(R.menu_layouts->system_menu_layout);
  midi_config_page.set_layout(R.menu_layouts->midiconfig_menu_layout);
  md_config_page.set_layout(R.menu_layouts->mdconfig_menu_layout);
  mcl_config_page.set_layout(R.menu_layouts->mclconfig_menu_layout);
  file_menu_page.set_layout(R.menu_layouts->file_menu_layout);
  seq_menu_page.set_layout(R.menu_layouts->seq_menu_layout);
  grid_slot_page.set_layout(R.menu_layouts->slot_menu_layout);
  wavdesign_menu_page.set_layout(R.menu_layouts->wavdesign_menu_layout);
  md_import_page.set_layout(R.menu_layouts->mdimport_menu_layout);
  start_menu_page.set_layout(R.menu_layouts->start_menu_layout);
  boot_menu_page.set_layout(R.menu_layouts->boot_menu_layout);
  midiport_menu_page.set_layout(R.menu_layouts->midiport_menu_layout);
  port1_menu_page.set_layout(R.menu_layouts->port1_menu_layout);
  port2_menu_page.set_layout(R.menu_layouts->port2_menu_layout);
  usbport_menu_page.set_layout(R.menu_layouts->usbport_menu_layout);
  midiprogram_menu_page.set_layout(R.menu_layouts->midiprogram_menu_layout);
  midiclock_menu_page.set_layout(R.menu_layouts->midiclock_menu_layout);
  midiroute_menu_page.set_layout(R.menu_layouts->midiroute_menu_layout);
  midimachinedrum_menu_page.set_layout(R.menu_layouts->midimachinedrum_menu_layout);
  midigeneric_menu_page.set_layout(R.menu_layouts->midigeneric_menu_layout);
  perf_menu_page.set_layout(R.menu_layouts->perf_menu_layout);
}

ResourceManager R;
