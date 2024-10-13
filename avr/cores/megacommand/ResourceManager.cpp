#include "MCL_impl.h"
#include "ResourceManager.h"

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
  // calibrate references
  R.page_entries->Entries[0].IconData = R.icons_page->icon_grid;
  R.page_entries->Entries[1].IconData = R.icons_page->icon_mixer;
  R.page_entries->Entries[2].IconData = R.icons_page->icon_perf;
  R.page_entries->Entries[3].IconData = R.icons_page->icon_route;


  R.page_entries->Entries[4].IconData = R.icons_page->icon_step;
  R.page_entries->Entries[5].IconData = R.icons_page->icon_lfo;
  R.page_entries->Entries[6].IconData = R.icons_page->icon_pianoroll;
  R.page_entries->Entries[7].IconData = R.icons_page->icon_chroma;


  uint8_t idx = 8;
#ifdef SOUND_PAGE
  R.page_entries->Entries[idx].IconData = R.icons_page->icon_sample;
  ++idx;
#endif
#ifdef WAV_DESIGNER
  R.page_entries->Entries[idx].IconData = R.icons_page->icon_wavd;
  ++idx;
#endif
#ifdef LOUDNESS_PAGE
  R.page_entries->Entries[idx].IconData = R.icons_page->icon_loudness;
  ++idx;
#endif

  R.page_entries->Entries[idx].IconData = R.icons_page->icon_rhytmecho;
  ++idx;
  R.page_entries->Entries[idx].IconData = R.icons_page->icon_gatebox;
  ++idx;
  R.page_entries->Entries[idx].IconData = R.icons_page->icon_ram1;
  ++idx;
  R.page_entries->Entries[idx].IconData = R.icons_page->icon_ram2;
  // calibration complete
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
    midiprogram_menu_page.set_layout(R.menu_layouts->midiprogram_menu_layout);
    midiclock_menu_page.set_layout(R.menu_layouts->midiclock_menu_layout);
    midiroute_menu_page.set_layout(R.menu_layouts->midiroute_menu_layout);
    midimachinedrum_menu_page.set_layout(R.menu_layouts->midimachinedrum_menu_layout);
    midigeneric_menu_page.set_layout(R.menu_layouts->midigeneric_menu_layout);
    perf_menu_page.set_layout(R.menu_layouts->perf_menu_layout);
}

ResourceManager R;
