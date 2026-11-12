/* Native kennel host binding for GIN by SquareWave Studio.
 * Copyright (C) 2026 Hundekraft. GPL-3.0-or-later with OP-1 firmware permission.
 * Instrument source notices/credits are preserved under vendor/. */
#include "../../op1_feature_drum_synth.h"
#include "gin_fw.h"
#include "gin_fw_snapshot.h"
#include "gin_controls.h"
#if GIN_SYNTH_VOICE_LIMIT != 4
#error Kennel_Gin_requires_four_synthesis_voices
#endif
#include "../../build/gin/gin_catalog.inc"
typedef int (*gin_catalog_cb)(void *,int,char **,char **);
uint32_t gin_fw_database_exec(const char *,gin_catalog_cb,void *);
uint32_t gin_fw_json_dbox(const void *,const void *);
uint32_t gin_fw_factory_preset_query(void *,void *,const char *);
uint32_t gin_fw_browser_preset_copy(void *,uint32_t,const void *);
static void reset(void){for(unsigned i=0;i<12;i++)gin_sdk_retire_voice(&gin_fw_runtime.binding[i]);}
static const op1_drum_synth_t instrument={
 .api=OP1_DRUM_SYNTH_API,.size=sizeof(op1_drum_synth_t),.name="gin",.state_key="gin_state",
 .type=(void *)gin_fw_type_entry,.factory=gin_fw_factory_parent,.route=gin_fw_route,
 .trigger=gin_fw_trigger,.render=gin_fw_render,.status=gin_fw_status,.release=gin_fw_release,
 .screen_ctor=gin_fw_ui_ctor,.screen_load=gin_fw_ui_load,.screen_select=gin_fw_ui_select,
 .key=gin_fw_ui_key,.knob=gin_fw_ui_knob,.json_equal=gin_fw_json_dbox,
 .preset_query=gin_fw_factory_preset_query,.preset_copy=gin_fw_browser_preset_copy,
 .snapshot_read=gin_fw_snapshot_read,.snapshot_save=gin_fw_snapshot_slot_save,
 .normalize=gin_fw_normalize,.service=gin_fw_working_tick,.reset=reset
};
static void event(int id,int32_t arg){
 (void)arg;
 if(id==OP1_EV_NOTES_RESET)reset();
 if(id==OP1_EV_BOOT_DONE){unsigned slot=*(volatile uint32_t *)0x01AAAC10;if(slot<8&&(gin_fw_selected_mask&(1u<<slot)))gin_fw_factory_parent(9);}
 if(id==OP1_EV_REGISTER){
  if(op1_drum_synth_register(&instrument)){op1_toast("GIN unavailable");return;}
  gin_fw_database_open();
  for(unsigned i=0;gin_catalog_sql[i];i++)if(gin_fw_database_exec(gin_catalog_sql[i],0,0)){op1_toast("GIN kits unavailable");break;}
  /* Restore identities for slots loaded before file registration. Construction
   * and coefficient publication still happen through the core's native factory. */
  for(unsigned slot=0;slot<8;slot++){
   const void *json=(const void *)(0x01DDD9ACu+slot*0x38);
   const void *v=((const void *(*)(const void *,const char *))0x019BEB70)(json,"type");
   gin_fw_string_t name={0};name.cap=15;
   if(v&&((const unsigned char *)v)[8]==4){const char *s=*(const char *const *)v;while(name.size<15&&s[name.size]){name.u.buf[name.size]=s[name.size];name.size++;}}
   /* Unused slots have no type; asking R246 to resolve an empty name asserts. */
   if(name.size)gin_fw_activation_type(&name,slot);
  }
 }
}
static const char *const claims[]={"drum:gin",0};
const op1_feature_t FEATURE_gin={.api=OP1_FEATURE_API,.priority=50,.name="gin",.event=event,.claims=claims,.drum_synth=&instrument};
