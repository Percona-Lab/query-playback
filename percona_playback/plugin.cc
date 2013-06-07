/* BEGIN LICENSE
 * Copyright (C) 2011-2013 Percona Ireland Ltd.
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 * END LICENSE */

#include <config.h>

#include <vector>
#include <cstdio>
#include <string>
#include <map>
#include <dlfcn.h>

#include <boost/foreach.hpp>
#include <percona_playback/plugin.h>
#include <percona_playback/plugin_load_list.h>
#include <percona_playback/tokenize.h>
#include <percona_playback/gettext.h>

/* plugin definition symbols */
typedef percona_playback::plugin::definition plugin_builtin_list[];
extern plugin_builtin_list PANDORA_BUILTIN_SYMBOLS_LIST;
extern plugin_builtin_list PANDORA_BUILTIN_LOAD_SYMBOLS_LIST;

/* lists of things we have, and to load */
percona_playback::plugin::definition *builtin_plugins[]= { PANDORA_BUILTIN_SYMBOLS_LIST, NULL };
percona_playback::plugin::definition *builtin_load_plugins[]= { PANDORA_BUILTIN_LOAD_SYMBOLS_LIST, NULL };

namespace percona_playback {

static void load_plugin(const char *so_file, const std::string &plugin_name)
{
    void *dl_handle= NULL;
    dl_handle= dlopen(so_file, RTLD_NOW|RTLD_LOCAL);
    if (dl_handle == NULL)
    {
      const char *errmsg= dlerror();
      fprintf(stderr,_("Error loading plugin: %s\n\n"), errmsg);
      abort();
      return;
    }

    std::string plugin_decl_sym("_percona_playback_");
    plugin_decl_sym.append(plugin_name);
    plugin_decl_sym.append("_plugin_");

    void *sym= dlsym(dl_handle, plugin_decl_sym.c_str());
    if (sym == NULL)
    {
      const char *errmsg= dlerror();
      fprintf(stderr,_("Error loading plugin: %s\n\n"), errmsg);
      abort();
      return;
    }

    const percona_playback::plugin::definition *def=
      static_cast<percona_playback::plugin::definition *>(sym);

    if (def)
      def->init(percona_playback::PluginRegistry::singleton());

    PluginRegistry::singleton().loaded_plugin_names.push_back(plugin_name);
}

void load_plugins()
{
  std::vector<std::string> builtin_load_list;
  std::vector<std::string> load_list;
  tokenize(PANDORA_BUILTIN_LOAD_LIST, builtin_load_list, ",", true);
  tokenize(PANDORA_PLUGIN_LIST, load_list, ",", true);

  /* load builtin plugins */
  BOOST_FOREACH(const std::string& plugin_name, builtin_load_list)
  {
    load_plugin(NULL, plugin_name);
  }

  /* load default plugins */
  BOOST_FOREACH(const std::string& plugin_name, load_list)
  {
    std::string plugin_lib_name("lib");
    plugin_lib_name.append(plugin_name);
    plugin_lib_name.append("_plugin");
#if defined(TARGET_OS_OSX)
    plugin_lib_name.append(".dylib");
#else
    plugin_lib_name.append(".so");
#endif
    load_plugin(plugin_lib_name.c_str(), plugin_name);
  }

}

} /* namespace percona_playback */
