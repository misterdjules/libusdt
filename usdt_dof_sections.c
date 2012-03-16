#include "usdt.h"

#include <stdlib.h>

usdt_dof_section_t *
usdt_dof_probes_sect(usdt_provider_t *provider, usdt_strtab_t *strtab)
{
        usdt_dof_section_t *probes;
        usdt_probedef_t *pd;
        usdt_probe_t *probe, *p;
        dof_stridx_t type, argv;
        uint8_t argc, i;
        uint32_t argidx = 0;
        uint32_t offidx = 0;
        void *dof;
        
        probes = malloc(sizeof(*probes));
        usdt_dof_section_init(probes, DOF_SECT_PROBES, 1);
  
        for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
                argc = 0;
                argv = 0;

                probe = malloc(sizeof(*probe));
    
                for (i = 0; pd->types[i] != USDT_ARGTYPE_NONE && i < 6; i++) {
                        probe->types[i] = pd->types[i];
      
                        switch(pd->types[i]) {
                        case USDT_ARGTYPE_INTEGER:
                                type = usdt_strtab_add(strtab, "int");
                                break;
                        case USDT_ARGTYPE_STRING:
                                type = usdt_strtab_add(strtab, "char *");
                                break;
                        }
      
                        argc++;
                        if (argv == 0)
                                argv = type;
                }

                probe->next     = NULL;
                probe->name     = usdt_strtab_add(strtab, pd->name);
                probe->func     = usdt_strtab_add(strtab, pd->function);
                probe->noffs    = 1;
                probe->enoffidx = offidx;
                probe->argidx   = argidx;
                probe->nenoffs  = 1;
                probe->offidx   = offidx;
                probe->nargc    = argc;
                probe->xargc    = argc;
                probe->nargv    = argv;
                probe->xargv    = argv;
    
                usdt_create_tracepoints(probe);
    
                argidx += argc;
                offidx++;
    
                pd->probe = probe;
    
                if (provider->probes == NULL) {
                        provider->probes = probe;
                }
                else {
                        for (p = provider->probes; (p->next != NULL); p = p->next) ;
                        p->next = probe;
                }

                dof = usdt_probe_dof(probe);
                usdt_dof_section_add_data(probes, dof, sizeof(dof_probe_t));
                free(dof);

                probes->entsize = sizeof(dof_probe_t);
        }

        return (probes);
}

usdt_dof_section_t *
usdt_dof_prargs_sect(usdt_provider_t *provider)
{
        usdt_dof_section_t *prargs;
        usdt_probedef_t *pd;
        uint8_t i;

        prargs = malloc(sizeof(*prargs));
        usdt_dof_section_init(prargs, DOF_SECT_PRARGS, 2);
        prargs->entsize = 1;

        for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
                for (i = 0; pd->types[i] != USDT_ARGTYPE_NONE && i < 6; i++)
                        usdt_dof_section_add_data(prargs, &i, 1);
        }
        if (prargs->size == 0) {
                i = 0;
                usdt_dof_section_add_data(prargs, &i, 1);
        }
        
        return (prargs);
}

usdt_dof_section_t *
usdt_dof_proffs_sect(usdt_provider_t *provider, char *dof)
{
        usdt_dof_section_t *proffs;
        usdt_probedef_t *pd;
        uint32_t off;

        proffs = malloc(sizeof(*proffs));
        usdt_dof_section_init(proffs, DOF_SECT_PROFFS, 3);
        proffs->entsize = 4;
  
        for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
                off = usdt_probe_offset(pd->probe, dof, usdt_probedef_argc(pd));
                usdt_dof_section_add_data(proffs, &off, 4);
        }
        
        return (proffs);
}

usdt_dof_section_t *
usdt_dof_prenoffs_sect(usdt_provider_t *provider, char *dof)
{
        usdt_dof_section_t *prenoffs;
        usdt_probedef_t *pd;
        uint32_t off;

        prenoffs = malloc(sizeof(*prenoffs));
        usdt_dof_section_init(prenoffs, DOF_SECT_PRENOFFS, 4);
        prenoffs->entsize = 4;

        for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
                off = usdt_is_enabled_offset(pd->probe, dof);
                usdt_dof_section_add_data(prenoffs, &off, 4);
        }

        return (prenoffs);
}

usdt_dof_section_t *
usdt_dof_provider_sect(usdt_provider_t *provider)
{
        usdt_dof_section_t *provider_s;
        dof_provider_t p;
    
        provider_s = malloc(sizeof(*provider_s));
        usdt_dof_section_init(provider_s, DOF_SECT_PROVIDER, 5);
  
        p.dofpv_strtab   = 0;
        p.dofpv_probes   = 1;
        p.dofpv_prargs   = 2;
        p.dofpv_proffs   = 3;
        p.dofpv_prenoffs = 4;
        p.dofpv_name     = 1; // provider name always first strtab entry.
        p.dofpv_provattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING,
                                    DTRACE_STABILITY_EVOLVING,
                                    DTRACE_STABILITY_EVOLVING);
        p.dofpv_modattr  = DOF_ATTR(DTRACE_STABILITY_PRIVATE,
                                    DTRACE_STABILITY_PRIVATE,
                                    DTRACE_STABILITY_EVOLVING);
        p.dofpv_funcattr = DOF_ATTR(DTRACE_STABILITY_PRIVATE,
                                    DTRACE_STABILITY_PRIVATE,
                                    DTRACE_STABILITY_EVOLVING);
        p.dofpv_nameattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING,
                                    DTRACE_STABILITY_EVOLVING,
                                    DTRACE_STABILITY_EVOLVING);
        p.dofpv_argsattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING,
                                    DTRACE_STABILITY_EVOLVING,
                                    DTRACE_STABILITY_EVOLVING);
  
        usdt_dof_section_add_data(provider_s, &p, sizeof(p));
        
        return (provider_s);
}
