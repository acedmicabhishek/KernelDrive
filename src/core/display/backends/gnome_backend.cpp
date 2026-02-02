#include "gnome_backend.h"
#include <gio/gio.h>
#include <iostream>
#include <cmath>

bool GnomeBackend::is_available() {
    const char* xdg = std::getenv("XDG_CURRENT_DESKTOP");
    return (xdg && (std::string(xdg).find("GNOME") != std::string::npos));
}


// Structure parsing for Mutter DBus
// references:
// https://github.com/jadahl/gnome-monitor-config/blob/master/src/org.gnome.Mutter.DisplayConfig.xml
// GetCurrentState () -> (u serial, a((ssssqqdddaja{sv})) monitors, a(iiqqvf) logical_monitors, a(iiiiii) properties)

std::vector<DisplayInfo> GnomeBackend::get_displays() {
    std::vector<DisplayInfo> infos;
    
    GError* error = nullptr;
    GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        "org.gnome.Mutter.DisplayConfig",
        "/org/gnome/Mutter/DisplayConfig",
        "org.gnome.Mutter.DisplayConfig",
        nullptr, &error
    );

    if (error) {
        g_error_free(error);
        return infos;
    }

    GVariant* ret = g_dbus_proxy_call_sync(proxy, "GetCurrentState", nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
    if (error) {
        g_error_free(error);
        g_object_unref(proxy);
        return infos;
    }

    guint32 serial;
    GVariantIter *monitors_iter;
    GVariantIter *logical_iter;
    GVariantIter *props_iter;
    
    g_variant_get(ret, "(ua((ssssqqdddaja{sv}))a(iiqqvf)a(iiiiii))", &serial, &monitors_iter, &logical_iter, &props_iter);

    GVariant* monitors = g_variant_get_child_value(ret, 1);
    GVariantIter iter;
    g_variant_iter_init(&iter, monitors);
    
    GVariant* monitor;
    while ((monitor = g_variant_iter_next_value(&iter))) {
        DisplayInfo info;

        GVariant* spec = g_variant_get_child_value(monitor, 0);
        const char *conn, *vend, *prod, *ser;
        g_variant_get(spec, "(&s&s&s&s)", &conn, &vend, &prod, &ser);
        info.id = conn;
        info.model = prod;
        info.make = vend;
        g_variant_unref(spec);
        
         GVariant* modes = g_variant_get_child_value(monitor, 1);
        GVariantIter mode_iter;
        g_variant_iter_init(&mode_iter, modes);
        
        GVariant* mode_val;
        while ((mode_val = g_variant_iter_next_value(&mode_iter))) {
            char* mode_id;
            gint32 w, h;
            double rate, pref_scale;
             g_variant_get(mode_val, "(siidd@ad@a{sv})", &mode_id, &w, &h, &rate, &pref_scale, NULL, NULL);
            
            DisplayMode dm;
            dm.width = w;
            dm.height = h;
            dm.refresh_rate_mHz = (int)(rate * 1000);
            
            info.available_modes.push_back(dm);
            g_free(mode_id);
            g_variant_unref(mode_val);
        }
        g_variant_unref(modes);
         info.enabled = true; 
        if (!info.available_modes.empty()) info.current_mode = info.available_modes[0];
        info.scale = 1.0;
        
        infos.push_back(info);
        g_variant_unref(monitor);
    }
    
    GVariant* logicals = g_variant_get_child_value(ret, 2);
    GVariantIter log_iter;
    g_variant_iter_init(&log_iter, logicals);
    GVariant* logical;
    while ((logical = g_variant_iter_next_value(&log_iter))) {
        
        gint32 x, y;
        double scale;
        guint32 transform;
        gboolean primary;
        GVariant* monitors_list;
        
        g_variant_get(logical, "(iidub@a(ssss))", &x, &y, &scale, &transform, &primary, &monitors_list);
        
        
        GVariantIter mon_list_iter;
        g_variant_iter_init(&mon_list_iter, monitors_list);
        GVariant* mon_spec;
        while ((mon_spec = g_variant_iter_next_value(&mon_list_iter))) {
             const char *c, *v, *p, *s;
             g_variant_get(mon_spec, "(&s&s&s&s)", &c, &v, &p, &s);
             
        
             for (auto& inf : infos) {
                 if (inf.id == c) {
                     inf.scale = scale;
                     inf.primary = primary;
                    
                 }
             }
             g_variant_unref(mon_spec);
        }
        g_variant_unref(monitors_list);
        g_variant_unref(logical);
    }
    
    g_variant_unref(monitors);
    g_variant_unref(logicals);
    g_variant_unref(ret);
    g_object_unref(proxy);
    
    return infos;
}

bool GnomeBackend::set_mode(const std::string& display_id, const DisplayMode& mode) {
   
    std::cerr << "GnomeBackend: Set Mode Not Fully Implemented (Requires Full Config Serializer)" << std::endl;
    return false;
}

bool GnomeBackend::set_scale(const std::string& display_id, double scale) {
    return false;
}
