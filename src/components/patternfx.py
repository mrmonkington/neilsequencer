import neil.com as com
from neil.utils import roundint, bn2mn, mn2bn

class Interpolate():
    name = 'Interpolate'

    __neil__ = dict(
        id = 'neil.core.patternfx.interpolate',
        categories = [
            'patternfx'
            ]
        )

    def transform(self, widget, plugin, pattern, selection, selection_range):
        player = com.get('neil.core.player')
        player.set_callback_state(False)
        if not selection:
            return
        if selection.end == selection.begin + 1:
            return
        step = 1
        for row, group, track, index in selection_range:
            if row > plugin.get_pattern_length(pattern) - 1:
                break
            if row < 0:
                continue
            parameter = plugin.get_parameter(group, track, index)
            v1 = plugin.get_pattern_value(pattern, group, track, index, 
                                          selection.begin)
            v2 = plugin.get_pattern_value(pattern, group, track, index, 
                                          selection.end - 1)
            if ((v1 != parameter.get_value_none()) and 
                (v2 != parameter.get_value_none())):
                if (parameter.get_type() == 0 and 
                    (v1 == zzub.zzub_note_value_off or 
                     v2 == zzub.zzub_note_value_off)):
                    continue
                if (row - selection.begin) % step != 0:
                    value = parameter.get_value_none()
                else:
                    f = (float(row - selection.begin) / 
                         float(selection.end - selection.begin - 1))
                    if (parameter.get_type() == 0):
                        v1 = bn2mn(v1)
                        v2 = bn2mn(v2)
                        value = mn2bn(roundint((v2 - v1) * f + v1))
                    else:
                        value = roundint((v2 - v1) * f + v1)
                plugin.set_pattern_value(pattern, group, track, index,
                                         row, value)
        player.history_commit("interpolate")
        if player.set_callback_state(True):
            eventbus = com.get('neil.core.eventbus')
            eventbus.document_loaded()

        

__all__ = [
    'Interpolate'
]

__neil__ = dict(
    classes = [
        Interpolate
        ]
)
