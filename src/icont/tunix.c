/*
 * tunix.c - user interface for Unix.
 */

#include "../h/gsupport.h"
#include "../h/version.h"
#include "tproto.h"
#include "tglobals.h"

static void execute (char *ofile, char *efile, char *args[]);
static void usage (void);
static char *libpath (char *prog, char *envname);

static void txrun(void (*func)(char*, char*), char *source, char *args[]);
static void copyfile(char *dstfile, char *srcfile);
static void savefile(char *dstfile, char *srcprog);
static void cleanup(void);

static char **rfiles;		/* list of files removed by cleanup() */

/*
 * for old method of hardwiring iconx path; not normally used.
 */
static char patchpath[MaxPath+18] = "%PatchStringHere->";

/*
 * getopt() variables
 */
extern int optind;		/* index into parent argv vector */
extern int optopt;		/* character checked for validity */
extern char *optarg;		/* argument associated with option */

/*
 *  main program
 */
int main(int argc, char *argv[]) {
   int nolink = 0;			/* suppress linking? */
   int errors = 0;			/* translator and linker errors */
   char **tfiles, **tptr;		/* list of files to translate */
   char **lfiles, **lptr;		/* list of files to link */
   char **rptr;				/* list of files to remove */
   char *efile = NULL;			/* stderr file */
   char buf[MaxFileName];		/* file name construction buffer */
   int c, n;
   char ch;
   struct fileparts *fp;

   /*
    * Initialize globals.
    */
   initglob();				/* general global initialization */
   ipath = libpath(argv[0], "IPATH");	/* set library search paths */
   lpath = libpath(argv[0], "LPATH");
   if (strlen(patchpath) > 18)
      iconxloc = patchpath + 18;	/* use stated iconx path if patched */
   else
      iconxloc = relfile(argv[0], "/../iconx");	/* otherwise infer it */

   /*
    * Process options. 
    * IMPORTANT:  When making changes here, also update usage() function.
    */
   while ((c = getopt(argc, argv, "+ce:f:o:stuv:ELP:VX:")) != EOF)
      switch (c) {
         case 'c':			/* -c: compile only (no linking) */
            nolink = 1;
            break;
         case 'e':			/* -e file: [undoc] redirect stderr */
            efile = optarg;
            break;
         case 'f':			/* -f features: enable features */
            if (strchr(optarg, 's') || strchr(optarg, 'a'))
               strinv = 1;		/* this is the only icont feature */
            break;
         case 'o':			/* -o file: name output file */
            ofile = optarg;
            break;
         case 's':			/* -s: suppress informative messages */
            silent = 1;
            verbose = 0;
            break;
         case 't':			/* -t: turn on procedure tracing */
            trace = -1;
            break;
         case 'u':			/* -u: warn about undeclared ids */
            uwarn = 1;
            break;
         case 'v':			/* -v n: set verbosity level */
            if (sscanf(optarg, "%d%c", &verbose, &ch) != 1)
               quitf("bad operand to -v option: %s", optarg);
            if (verbose == 0)
               silent = 1;
            break;
         case 'E':			/* -E: preprocess only */
            pponly = 1;
            nolink = 1;
            break;
         case 'P':			/* -P program: execute from argument */
            txrun(savefile, optarg, &argv[optind]);
            break;			/*NOTREACHED*/
         case 'V':
            fprintf(stderr, "%s  (%s, %s)\n", Version, Config, __DATE__);
            break;
         case 'X':			/* -X srcfile: execute single srcfile */
            txrun(copyfile, optarg, &argv[optind]);
            break;			/*NOTREACHED*/

#ifdef DeBugLinker
         case 'L':			/* -L: enable linker debugging */
            Dflag = 1;
            break;
#endif					/* DeBugLinker */

         default:
         case 'x':			/* -x illegal until after file list */
            usage();
         }

   /*
    * Allocate space for lists of file names.
    */
   n = argc - optind + 1;
   tptr = tfiles = (char **)alloc((unsigned int)(n * sizeof(char *)));
   lptr = lfiles = (char **)alloc((unsigned int)(n * sizeof(char *)));
   rptr = rfiles = (char **)alloc((unsigned int)(2 * n * sizeof(char *)));

   /*
    * Scan file name arguments.
    */
   while (optind < argc)  {
      if (strcmp(argv[optind], "-x") == 0)	/* stop at -x */
         break;
      else if (strcmp(argv[optind], "-") == 0) {
         *tptr++ = "-";				/* "-" means standard input */
         *lptr++ = *rptr++ = "stdin.u1";
         *rptr++ = "stdin.u2";
         }
      else {
         fp = fparse(argv[optind]);		/* parse file name */
         if (*fp->ext == '\0' || smatch(fp->ext, SourceSuffix)) {
            makename(buf, SourceDir, argv[optind], SourceSuffix);
            *tptr++ = salloc(buf);		/* translate the .icn file */
            makename(buf, TargetDir, argv[optind], U1Suffix);
            *lptr++ = *rptr++ = salloc(buf);	/* link & remove .u1 */
            makename(buf, TargetDir, argv[optind], U2Suffix);
            *rptr++ = salloc(buf);		/* also remove .u2 */
            }
         else if (smatch(fp->ext, U1Suffix) || smatch(fp->ext, U2Suffix)
               || smatch(fp->ext, USuffix)) {
            makename(buf, TargetDir, argv[optind], U1Suffix);
            *lptr++ = salloc(buf);
            }
         else
            quitf("bad argument %s", argv[optind]);
         }
      optind++;
      }

   *tptr = *lptr = *rptr = NULL;	/* terminate filename lists */
   if (lptr == lfiles)
      usage();				/* error -- no files named */

   /*
    * Translate .icn files to make .u1 and .u2 files.
    */
   if (tptr > tfiles) {
      if (!pponly)
         report("Translating");
      errors = trans(tfiles, TargetDir);
      if (errors > 0)			/* exit if errors seen */
         exit(EXIT_FAILURE);
      }

   /*
    * Link .u1 and .u2 files to make an executable.
    */
   if (nolink)				/* exit if no linking wanted */
      exit(EXIT_SUCCESS);

   if (ofile == NULL)  {		/* if no -o file, synthesize a name */
      ofile = salloc(makename(buf, TargetDir, lfiles[0], IcodeSuffix));
      }
   else {				/* add extension in necessary */
      fp = fparse(ofile);
      if (*fp->ext == '\0' && *IcodeSuffix != '\0') /* if no ext given */
         ofile = salloc(makename(buf, NULL, ofile, IcodeSuffix));
   }

   report("Linking");
   errors = ilink(lfiles, ofile);	/* link .u files to make icode file */

   /*
    * Finish by removing intermediate files.
    *  Execute the linked program if so requested and if there were no errors.
    */
   cleanup();				/* delete intermediate files */
   if (errors > 0) {			/* exit if linker errors seen */
      remove(ofile);
      exit(EXIT_FAILURE);
      }
   if (optind < argc)  {
      report("Executing");
      execute (ofile, efile, argv + optind + 1);
      }
   exit(EXIT_SUCCESS);
   return 0;
   }

/*
 * execute - execute iconx to run the icon program
 */
static void execute(char *ofile, char *efile, char *args[]) {
   int n;
   char **argv, **p;

   for (n = 0; args[n] != NULL; n++)	/* count arguments */
      ;
   p = argv = (char **)alloc((unsigned int)((n + 5) * sizeof(char *)));

   *p++ = ofile;			/* pass icode file name */
   while ((*p++ = *args++) != 0)	/* copy args into argument vector */
      ;
   *p = NULL;

   /*
    * Redirect stderr if requested, then execute the file.
    * It knows how to find iconx.
    */
   if (efile != NULL) {
      close(fileno(stderr));
      if (strcmp(efile, "-") == 0)
         dup(fileno(stdout));
      else if (freopen(efile, "w", stderr) == NULL)
         quitf("could not redirect stderr to %s\n", efile);
      }
   execv(ofile, argv);
   quitf("could not execute %s", ofile);
   }

void report(char *s) {
   char *c = (strchr(s, '\n') ? "" : ":\n") ;
   if (!silent)
      fprintf(stderr, "%s%s", s, c);
   }

/*
 * Print an error message if called incorrectly.  The message depends
 *  on the legal options for this system.
 */
static void usage(void) {
   fprintf(stderr, "usage: %s %s\n",
      progname, "[-cstuEV] [-f s] [-o ofile] [-v i] file ... [-x args]");
   fprintf(stderr, "       %s -X sourcefile [args]\n", progname);
   fprintf(stderr, "       %s -P 'program'  [args]\n", progname);
   exit(EXIT_FAILURE);
   }

/*
 * Return path after appending lib directory.
 */
static char *libpath(char *prog, char *envname) {
   char buf[1000], *s;

   s = getenv(envname);
   if (s != NULL)
      strcpy(buf, s);
   else
      strcpy(buf, ".");
   strcat(buf, ":");
   strcat(buf, relfile(prog, "/../../lib"));
   return salloc(buf);
   }

/*
 * Translate, link, and execute a source file.
 * Does not return under any circumstances.
 */
static void txrun(void (*func)(char*, char*), char *source, char *args[]) {
   int omask;
   char c1, c2;
   char *flist[2];
   char srcfile[MaxFileName], u1[MaxFileName], u2[MaxFileName];
   char icode[MaxFileName], buf[MaxFileName + 20];
   static char abet[] = "abcdefghijklmnopqrstuvwxyz";

   silent = 1;			/* don't issue commentary while translating */
   omask = umask(0077);		/* remember umask; keep /tmp files private */

   /*
    * Invent a file named /tmp/innnnnxx.icn and copy the source code there.
    */
   srand(time(NULL));
   c1 = abet[rand() % (sizeof(abet) - 1)];
   c2 = abet[rand() % (sizeof(abet) - 1)];
   sprintf(srcfile, "/tmp/i%d%c%c.icn", getpid(), c1, c2);
   func(srcfile, optarg);

   /*
    * Derive other names and arrange for cleanup on exit.
    */
   rfiles = alloc(5 * sizeof(char *));
   rfiles[0] = srcfile;
   makename(rfiles[1] = u1, NULL, srcfile, U1Suffix);
   makename(rfiles[2] = u2, NULL, srcfile, U2Suffix);
   makename(rfiles[3] = icode, NULL, srcfile, IcodeSuffix);
   rfiles[4] = NULL;
   atexit(cleanup);

   /*
    * Translate to produce .u1 and .u2 files.
    */
   flist[0] = srcfile;
   flist[1] = NULL;
   if (trans(flist, SourceDir) > 0)
      exit(EXIT_FAILURE);

   /*
    * Link to make an icode file.
    */
   flist[0] = u1;
   if (ilink(flist, icode) > 0)
      exit(EXIT_FAILURE);

   /*
    * Execute the icode file.
    */
   rfiles[3] = NULL;			/* don't delete icode yet */
   cleanup();				/* but delete the others */
   sprintf(buf, "DELETE_ICODE_FILE=%s", icode);
   putenv(buf);				/* tell iconx to delete icode */
   umask(omask);			/* reset original umask */
   execute(icode, NULL, args);		/* execute the program */
   quitf("could not execute %s", icode);
   }

/*
 * Dump a string to a file, prefixed by  $line 0 "[inline]".
 */
static void savefile(char *dstfile, char *srcprog) {
   FILE *f = fopen(dstfile, "w");
   if (f == NULL)
      quitf("cannot open for writing: %s", dstfile);
   fprintf(f, "$line 0 \"[inline]\"\n");
   fwrite(srcprog, 1, strlen(srcprog), f);
   fclose(f);
   }

/*
 * Copy a source file for later translation, adding  $line 0 "filename".
 */
static void copyfile(char *dstfile, char *srcfile) {
   int c;
   FILE *e = fopen(srcfile, "r");
   FILE *f = fopen(dstfile, "w");
   if (e == NULL)
      quitf("cannot open: %s", srcfile);
   if (f == NULL)
      quitf("cannot open for writing: %s", dstfile);
   fprintf(f, "$line 0 \"%s\"\n", srcfile);
   while ((c = getc(e)) != EOF)
      putc(c, f);
   fclose(f);
   }

/*
 * Deletes the files listed in rfiles[].
 */
static void cleanup(void) {
   char **p;

   for (p = rfiles; *p; p++)
      remove(*p);			
}
