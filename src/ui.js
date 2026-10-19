/*
 * Tenebrae -- the module's own screen (preset and octave), from the shared
 * sound-generator base. The parameter pages are not drawn here: the host
 * builds them from the ui_hierarchy the plugin serves.
 */
import { createSoundGeneratorUI } from '/data/UserData/schwung/shared/sound_generator_ui.mjs';

const ui = createSoundGeneratorUI({
    moduleName: 'Tenebrae',
    onPresetChange: () => {
        host_module_set_param('all_notes_off', '1');
    },
    onOctaveChange: () => {
        host_module_set_param('all_notes_off', '1');
    },
    showPolyphony: false,
    showOctave: true,
});

globalThis.init = ui.init;
globalThis.tick = ui.tick;
globalThis.onMidiMessageInternal = ui.onMidiMessageInternal;
globalThis.onMidiMessageExternal = ui.onMidiMessageExternal;
