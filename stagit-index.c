#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *relpath = "";

static char description[255] = "Repositories";
static char *name = "";

void
joinpath(char *buf, size_t bufsiz, const char *path, const char *path2)
{
	int r;

	r = snprintf(buf, bufsiz, "%s%s%s",
		path, path[0] && path[strlen(path) - 1] != '/' ? "/" : "", path2);
	if (r < 0 || (size_t)r >= bufsiz)
		errx(1, "path truncated: '%s%s%s'",
			path, path[0] && path[strlen(path) - 1] != '/' ? "/" : "", path2);
}

/* Escape characters below as HTML 2.0 / XML 1.0. */
void
xmlencode(FILE *fp, const char *s, size_t len)
{
	size_t i;

	for (i = 0; *s && i < len; s++, i++) {
		switch(*s) {
		case '<':  fputs("&lt;",   fp); break;
		case '>':  fputs("&gt;",   fp); break;
		case '\'': fputs("&#39;" , fp); break;
		case '&':  fputs("&amp;",  fp); break;
		case '"':  fputs("&quot;", fp); break;
		default:   fputc(*s, fp);
		}
	}
}

void
writeheader(FILE *fp)
{
	fputs("<!DOCTYPE html>\n"
		"<html lang=\"en\">\n<head>\n"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
		"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
		"<title>", fp);
	xmlencode(fp, description, strlen(description));
	fprintf(fp, "</title>\n<link rel=\"icon\" type=\"image/png\" href=\"%sfavicon.png\" />\n", relpath);
	fprintf(fp, "<link rel=\"stylesheet\" type=\"text/css\" href=\"%sstyle.css\" />\n", relpath);
	fputs("</head>\n<body>\n<h1>", fp);
	xmlencode(fp, description, strlen(description));
	fputs("</h1>\n<main>\n<dl>\n", fp);
}

void
writefooter(FILE *fp)
{
	fputs("</dl>\n</main>\n</body>\n</html>\n", fp);
}

int
writerepo(FILE *fp)
{
	char *stripped_name = NULL, *p;

	/* strip .git suffix */
	if (!(stripped_name = strdup(name)))
		err(1, "strdup");
	if ((p = strrchr(stripped_name, '.')))
		if (!strcmp(p, ".git"))
			*p = '\0';

	fputs("<dt><a href=\"", fp);
	xmlencode(fp, stripped_name, strlen(stripped_name));
	fputs("/log.html\">", fp);
	xmlencode(fp, stripped_name, strlen(stripped_name));
	fputs("</a></dt>\n<dd>", fp);
	xmlencode(fp, description, strlen(description));
	fputs("</dd>\n", fp);

	free(stripped_name);

	return 0;
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	char path[PATH_MAX], repodirabs[PATH_MAX + 1];
	const char *repodir;
	int i;

	if (argc < 2) {
		fprintf(stderr, "%s [repodir...]\n", argv[0]);
		return 1;
	}

#ifdef __OpenBSD__
	for (i = 1; i < argc; i++)
		if (unveil(argv[i], "r") == -1)
			err(1, "unveil: %s", argv[i]);

	if (pledge("stdio rpath", NULL) == -1)
		err(1, "pledge");
#endif

	writeheader(stdout);

	for (i = 1; i < argc; i++) {
		repodir = argv[i];
		if (!realpath(repodir, repodirabs))
			err(1, "realpath");

		/* use directory name as name */
		if ((name = strrchr(repodirabs, '/')))
			name++;
		else
			name = "";

		/* read description or .git/description */
		joinpath(path, sizeof(path), repodir, "description");
		if (!(fp = fopen(path, "r"))) {
			joinpath(path, sizeof(path), repodir, ".git/description");
			fp = fopen(path, "r");
		}
		description[0] = '\0';
		if (fp) {
			if (!fgets(description, sizeof(description), fp))
				description[0] = '\0';
			fclose(fp);
		}
		writerepo(stdout);
	}
	writefooter(stdout);

	return 0;
}
