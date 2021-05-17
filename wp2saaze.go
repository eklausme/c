// Convert WordPress export format to Saaze markdown
// Based on wp2hugo.go
// Elmar Klausmeier, 18-Apr-2021

package main

import (
	"bufio"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"strings"
	"unicode"
)

// Global variables
type attachm_t struct { // almost always images, but can be doc, pdf, etc.
	cnt       int    // counter, starts with 0
	fname     string // file name, i.e., basename
	hugoFname string // fname prefixed with /img/
}

var attachm = make(map[string]*attachm_t) // counts number of occurences of attachments

var wpconfig = make(map[string]string) // goes to config.toml

// WordPress unfortunately inserts HTML codes into bash/C/R code.
// Have to get rid of these again.
var delimList = []struct {
	start string
	stop  string
	special int
}{
	{"[googlemaps ", "]", 0},
	{"[code", "[/code]", 0},
	{"<code>", "</code>", 0},
	{"<pre>", "</pre>", 0},
	{"$latex ", "$", 1},
	{"<a href=", ">", 2},
	{"<img src=", ">", 2},
}

func deHTMLify(s string) string {
	// glitch for WordPress foolishly changing "&" to "&amp;", ">" to "&gt;", etc.
	for _, v := range delimList {
		tx0, tx1, lenvstart, lenvstop := 0, 0, len(v.start), len(v.stop)
		for {
			if tx1 = strings.Index(s[tx0:], v.start); tx1 < 0 {
				break
			}
			if tx2 := strings.Index(s[tx0+tx1+lenvstart:], v.stop); tx2 > 0 {
				//fmt.Println("\t\tv =",v,", tx0 =",tx0,", tx1 =",tx1,", tx2 =",tx2,"\n\t\ts =",s[tx1:tx1+70])
				t := strings.Replace(s[tx0:tx0+tx1+tx2], "&amp;", "&", -1)
				if v.special != 2 {
					t = strings.Replace(t, "&lt;", "<", -1)
					t = strings.Replace(t, "&gt;", ">", -1)
					t = strings.Replace(t, "&quot;", "\"", -1)
					t = strings.Replace(t, "&#092;", "\\", -1)
					t = strings.Replace(t, "&#048;", "0", -1)
				}
				//if v.special == 1 {
				//	t = strings.Replace(t, `\{`, `\lbrace `, -1)
				//	t = strings.Replace(t, `\}`, `\rbrace `, -1)
				//}
				s = s[0:tx0] + t + s[tx0+tx1+tx2:]
			} else {
				u := len(s[tx0+tx1:])
				if u > 40 {
					u = 40
				}
				// Show up to 40 chars of offending string
				fmt.Println("\tClosing tag", v.stop, " in ", s[tx0+tx1:tx0+tx1+u], " not found")
			}
			tx0 += tx1 + lenvstart + lenvstop // + len(t)
		}
	}

	return s
}

// Very early simple search and replace via strings.ReplaceAll(), specific to Elmar Klausmeier
var earlySearchReplace = []struct {
	search string
	replace  string
}{
	{ "Starting J-Pilot shows:\n", "Starting J-Pilot shows:\n[code lang=text]\n" },
	{ "how may entries in datebook", "how many entries in datebook" },
	{ "Greenest data center in the word", "Greenest data center in the world" },
	{ "Curious what might cause this obscene activity, noticed that", "Curious what might cause this obscene activity, I noticed that" },
	{ "I type <code></code> to stop the", "I type <code>sk</code> to stop the" },
	{ `img src="//s0.wp.com`, `img src="https://s0.wp.com` },
	{ "http://latimesphoto.files.wordpress.com/2013/09/edward-burtynsky-water011.jpg", "../../../img/H2O_CHNA_DAM_XIA_01_11_big.jpg" },
	{ "www.uni-graz.at/~liebma/", "https://www.researchgate.net/profile/Manfred-Liebmann" },
	{ "http://marcusnoack.com/", "https://www.marcusnoack.com/" },
	{ "https://www.itc.rwth-aachen.de/go/id/epvp/gguid/0x5069F492F62D6046B0BFBB086838112B/allou/1/", "https://www.researchgate.net/profile/Dirk-Schmidl" },
	{ "http://upload.wikimedia.org/wikipedia/commons/thumb/8/86/Microsoft_Excel_2013_logo.svg/110px-Microsoft_Excel_2013_logo.svg.png", "https://en.wikipedia.org/wiki/Microsoft_Excel#/media/File:Microsoft_Office_Excel_(2019%E2%80%93present).svg" },
	{ "http://cs.sjsu.edu/~mak/CS185C/KnuthStructuredProgrammingGoTo.pdf", "https://pic.plover.com/knuth-GOTO.pdf" },
	{ "o1dBg__wsuo", "yVGYcSU89Q0" },	// Hilary Hahn, Mendelssohn, Frankfurt Radio Symphony Orchestra
	{ "\nBut otherwise this does not seem to indicate", "\n[/code]\nBut otherwise this does not seem to indicate" },
	{ `<code><a href="https://www.google.com/search?client=firefox-b-d&amp;q=speedtest">google.com/search?client=firefox-b-d&amp;q=speedtest</a></code>`, `<a href="https://www.google.com/search?client=firefox-b-d&amp;q=speedtest"><code>google.com/search?client=firefox-b-d&amp;q=speedtest</code></a>` },
	{ `<a href="https://eklausmeier.files.wordpress.com/2015/06/screenshot-from-2014-12-27-18-12-02.png"><img src="https://eklausmeier.files.wordpress.com/2015/06/screenshot-from-2014-12-27-18-12-02.png" alt="img src=&quot;https://lh3.googleusercontent.com/-Qt4lveCJG_0/VJ7qg9phJ1I/AAAAAAAAR18/jihNvXB3fdE/w633-h599-no/Screenshot%2Bfrom%2B2014-12-27%2B18%3A12%3A02.png&quot; alt=&quot;64 GB SanDisk Ultra USB thumb drive&quot; /&gt;&lt;a href=&quot;https://eklausmeier.files.wordpress.com/2015/06/screenshot-from-2014-12-27-18-12-02.png&quot;&gt;&lt;img src=&quot;https://eklausmeier.files.wordpress.com/2015/06/screenshot-from-2014-12-27-18-12-02.png&quot; alt=&quot;Screenshot from 2014-12-27 18-12-02&quot; width=&quot;633&quot; height=&quot;599&quot; class=&quot;aligncenter size-full wp-image-3025&quot; /&gt;&lt;/a&gt;" width="633" height="599" class="aligncenter size-full wp-image-3025" /></a>`, "![](../../../img/Screenshot%20from%202014-12-27%2018-12-02.png)" },
	{ `<table><tr><td>
<img src="https://pbs.twimg.com/media/CHs359CUMAIN1q1.jpg" alt="" />
<td><img src="https://pbs.twimg.com/media/CHs36BPVEAAUA7j.jpg" alt="" />
</table>

<table><tr><td>
<img src="https://pbs.twimg.com/media/CF7DxSlUsAAnxWM.png" alt="" />
<td><img src="https://pbs.twimg.com/media/CF7Dw9YVAAAeVvG.jpg" alt="" />
</table>
`, `
   |   |
---|---|---|---|---
![](https://pbs.twimg.com/media/CHs359CUMAIN1q1.jpg) | ![](https://pbs.twimg.com/media/CHs36BPVEAAUA7j.jpg)
![](https://pbs.twimg.com/media/CF7DxSlUsAAnxWM.png) | ![](https://pbs.twimg.com/media/CF7Dw9YVAAAeVvG.jpg)

` },
	{ `
<table>
<tr><td>$latex x$<td>$latex 1$<td>$latex 2$<td>$latex 2\cdot 2$<td>$latex 2\cdot 2\cdot 2$<td>$latex \cdots$
<tr><td>$latex y$<td>$latex a$<td>$latex q\cdot a$<td>$latex q\cdot q\cdot a$<td>$latex q\cdot q\cdot q\cdot a$<td>$latex \cdots$
</table>
`, `
$$
\begin{array} {|c|c|c|c|c|}\hline
x & 1 & 2 & 2\cdot 2 & 2\cdot 2\cdot 2 & \cdots \\ \hline
y & a & q\cdot a & q\cdot q\cdot a & q\cdot q\cdot q\cdot a & \cdots \\ \hline
\end{array}
$$
` },
}

// Simple search and replace via strings.ReplaceAll()
var searchReplace = []struct {
	search string
	replace  string
}{
	// replace hyperlinks from baseURL to root
	// this logic is not perfect if href points to page, instead of post
	// "<a href=\""+wpconfig["baseURL"], "<a href=\"/post", -1)
	// "<a href=\""+wpconfig["baseURLalt"], "<a href=\"/post/", -1)

	// Probably specific to Elmar Klausmeier's blog
	{ `<!--more-->`, "[more_WP_Tag]" },
	{ "}$\n<p style", "\n$$\n<p style" },	// Elmar specific: must come before the next ones
	{ `<p style="text-align:center;">$latex \displaystyle{`, "$$" },
	{ `<p style="text-align:center;">$latex {`, "$$" },
	{ `<p style="text-align:center;">$latex`, "$$" },
	{ "}$</p>", "$$" },
	{ "$</p>\n", "$$\n" },
	{ "\n$</p>", "\n$$" },
	// Very specific to Elmar's blog content: corrections in blog-post
	{ "$linear combination / interpolation polynomial with previous grid elements</p>\n", "\\hbox{linear combination / interpolation polynomial with previous grid elements}$$\n" },
	{ " to  mathbb{", ` \to\mathbb{` },
	{ " mathbb{", " \\mathbb{" },
	{ `(-infty, 0) cup (1, infty)`, `(-\infty, 0) \cup (1, \infty)` },
	{ "} to (0,1)", `} \to (0,1)` },
	//{ ") to mathbb{", `) \to \mathbb{` },
	{ "latex square$", `latex \square` },
	{ "\n<p style=\"text-align:right;\">", " " },
	// Get rid of the artificial WordPress ugliness
	{ "<!-- wp:paragraph -->\n<p><br>", "" },
	{ "<!-- wp:paragraph -->\n<p>", "" },
	{ "</p>\n<!-- /wp:paragraph -->", "" },
	{ "<!-- wp:more -->\n", "" },
	{ "<!-- /wp:more -->\n", "" },
	{ "<!-- wp:preformatted -->\n<pre class=\"wp-block-preformatted\">", "" },
	{ "</pre>\n<!-- /wp:preformatted -->\n", "" },
	{ "<!-- wp:list {\"ordered\":true} -->\n<ol>", "<ol>\n\t" },
	{ "</ol>\n<!-- /wp:list -->\n", "\n</ol>\n" },
	{ "</li><li>", "</li>\n\t<li>" },	// will be replaced again below
	{ "</div>\n<!-- /wp:image -->", "" },
	{ "<!-- wp:table -->\n<figure class=\"wp-block-table\">", "\n" },
	{ "</figure>\n<!-- /wp:table -->\n", "\n" },
	{ "</tr><tr>", "</tr>\n<tr>" },
	{ "[/caption]", "" },
	{ "\n<P>\n", "\n\n" },
	//s = strings.ReplaceAll(s, "", "" },
	//replaceSpecChar(s)
	// HTML to markdown
	{ "<ol>\n", "\n" },
	{ "<ol>\t", "\n\t\t" },	// tricky: extra \t because dropped again by below \t<li>
	{ "</ol>\n", "\n" },
	{ "<ul>\n", "\n" },
	{ "</ul>\n", "\n" },
	{ "</li>\n", "\n" },
	// We loose unordered lists
	{ "\t<li>", "1. " },
}

// Use regexp to change various WordPress specific codes and
// map them to the equivalent Hugo codes.
// This list should be put into a configuration file.
var srchReplRegEx = []struct {
	regx    *regexp.Regexp
	replace string
}{
	// convert [code lang=bash] to ```bash
	{regexp.MustCompile("(\n{0,1})\\[code\\s*lang(uage|)=(\"|)([A-Za-z\\+]+)(\"|)(.*)\\]\\w*\n"), "\n```$4$6\n"},
	// convert [code] to ```
	{regexp.MustCompile("(\n{0,1})\\[code]\\w*\n"), "\n```\n"},
	// convert [/code] to ```
	{regexp.MustCompile("\n\\[/code\\]\\s*\n"), "\n```\n"},
	// handle https://www.youtube.com/watch?v=wtqfC9v0xB0
	{regexp.MustCompile("\nhttp(.|)://www\\.youtube\\.com/watch\\?v=([\\-\\w]+)(|&.+?])\n"), "\n[youtube] $2 [/youtube]\n"},
	// handle [youtube=http://www.youtube.com/watch?v=IA8X1cXFo9oautoplay=0&start=0&end=0]
	{regexp.MustCompile(`\[youtube=http(.|)://www\.youtube\.com/watch\?v=([\-\w]+)(&.+|)\]`), "[youtube] $2 [/youtube]"},
	// handle [vimeo 199882338]
	{regexp.MustCompile(`\[vimeo (\d\d\d+)\]`), "{{< vimeo $1 >}}"},
	// handle [vimeo https://vimeo.com/167845464]
	{regexp.MustCompile(`\[vimeo http(.|)://vimeo\.com/(\d\d\d+)\]`), "{{< vimeo $2 >}}"},
	// convert <code>, which is used as <pre>, to ```
	{regexp.MustCompile("\n<code>\\s*\n"), "\n```\n"},
	// handle </code> which is used as </pre>
	{regexp.MustCompile("\n</code>\\s*\n"), "\n```\n"},
	// convert <pre> and </pre> to ```
	{regexp.MustCompile("(\n{0,1})<(/|)pre>\\s*(\n{0,1})"), "\n```\n"},
	// convert $latex ...$ to `$...$`, handle multiline matches with (?s)
	// https://golang.org/pkg/regexp/#Regexp: To insert a literal $ in the output, use $$ in the template.
	{regexp.MustCompile(`\$latex\s+(?s)(.+?)\$`), "$$${1}$$"},
	// convert [googlemaps ...] to <iframe ...>/<iframe>
	{regexp.MustCompile(`\[googlemaps\s+(.+)\]`), `<iframe src="$1"></iframe>`},
	// convert <code>xxx</code> to `xxx`, non-greedy
	{regexp.MustCompile("<code>(.+?)</code>"), "`$1`"},
	// convert <h1>xxx</h1> to # xxx
	{regexp.MustCompile("<h1>(.+?)</h1>\n"), "# $1\n"},
	// convert <h2>xxx</h2> to ## xxx
	{regexp.MustCompile("<h2>(.+?)</h2>\n"), "## $1\n"},
	// convert <h3>xxx</h3> to ### xxx
	{regexp.MustCompile("<h3>(.+?)</h3>\n"), "### $1\n"},
	// convert <h4>xxx</h4> to #### xxx
	{regexp.MustCompile("<h4>(.+?)</h4>\n"), "#### $1\n"},
	// convert <h5>xxx</h5> to ##### xxx
	{regexp.MustCompile("<h5>(.+?)</h5>\n"), "##### $1\n"},
	// convert <h6>xxx</h6> to ###### xxx
	{regexp.MustCompile("<h6>(.+?)</h6>\n"), "###### $1\n"},
	// Elmar Klausmeier specific relative posts and images
	{regexp.MustCompile(` href="http(s|)://eklausmeier.wordpress.com/(\d\d\d\d)/(\d\d)/(\d\d)/`), ` href="../../$2/$3-$4-`},
	{regexp.MustCompile(` src="http(s|)://eklausmeier.files.wordpress.com/\d\d\d\d/\d\d/\d\d/([\-\.\w]+?)(|\?.+?)"`), ` src="../../../img/$2"`},
	{regexp.MustCompile(` src="http(s|)://eklausmeier.files.wordpress.com/\d\d\d\d/\d\d/([\-\.\w]+?)(|\?.+?)"`), ` src="../../../img/$2"`},
	{regexp.MustCompile("\nhttps://lh\\d.+?/IMG_20([_\\d]+)\\.jpg\n"), "\n![](../../../img/IMG_20$1.jpg)\n" },
	{regexp.MustCompile("https://lh\\d\\.googleusercontent.+?/IMG_20([_\\d]+)\\.jpg"), "../../../img/IMG_20$1.jpg" },
	// WordPress changed case of image files
	{regexp.MustCompile(`/img/screenshot-from-(\d\d\d\d-\d\d-\d\d)-(\d\d-\d\d-\d\d)`), `/img/Screenshot%20from%20$1%20$2`},
	// WordPress ugliness
	{regexp.MustCompile("\\[caption .+?\\]"), ""},
	{regexp.MustCompile("<!-- wp:image {\"align\":\"center\",\"id\":\\d+,\"linkDestination\":\"custom\"} -->\n<div class=\"wp-block-image\">"), ""},
	//  {regexp.MustCompile("<figure class=\"[-\\ \\w]+?\"><a href=\".+?\"><img src=(.+?)></a></figure>"), "![]($1)"},
	//  {regexp.MustCompile(`<a href=".+?" rel=".+?"><img src="(.+?)" alt="(.*?)" width="(.+?)" height="(.+?)" class(.*?)>.+?</a>`), "![$2]($1)"},
	{regexp.MustCompile(`<a href=".+?"><img src="(.+?)" (|width=".+?" )alt="(.*?)".*?>.*?</a>`), "![$3]($1)"},
	{regexp.MustCompile(`<a href=".+?"><img class=".+?" src="(.+?)" alt="(.*?)" width="(.+?)" height="(.+?)"(| /)>.*?</a>`), "![$2]($1)"},
	{regexp.MustCompile(`<a href=".+?"><img class=".+?" alt="(.*?)" src="(.+?)" width="(.+?)" height="(.+?)"(| /)>.*?</a>`), "![$1]($2)"},
	{regexp.MustCompile("<a href=\".+?\"><img src=(.+?)></a>"), "![]($1)"},
	// HTML to markdown, order is important
	{regexp.MustCompile(`<a href="([^ ]+?)" title="(.+?)">(.+?)</a>`), `[$3]($1 "$2")`},
	{regexp.MustCompile(`<a title="([^ ]+?)" href="(.+?)">(.+?)</a>`), `[$3]($2 "$1")`},
	{regexp.MustCompile(`<a href="([^ ]+?)">(.+?)</a>`), `[$2]($1)`},
	//  {regexp.MustCompile(`<img src="(.+?)" alt="(.*?)" width="(.+?)" height="(.+?)" class(.*?)>`), `![$2]($1)`},
	{regexp.MustCompile(`<img src="(.+?)" alt="(.*?)".*?>`), `![$2]($1)`},
	{regexp.MustCompile(`<img alt="(.*?)" src="(.+?)"(| /)>`), `![$1]($2)`},
	{regexp.MustCompile(`<img src="(.+?)" width="(.+?)" height="(.+?)" class(.*?)>`), `![]($1)`},
	{regexp.MustCompile("<figure.*?>(.+?)</figure>"), "$1"},
	//{regexp.MustCompile(""), ""},
}

// Late search and replace via strings.ReplaceAll()
// WordPress mixes up case and sometimes adds artificial numbers.
// Below filenames are very specific to Elmar Klausmeier's blog.
var lateSearchReplace = []struct {
	search string
	replace  string
}{
	{ "/img/dalvikart", "/img/dalvikART" },
	{ "/img/hotsync", "/img/HotSync" },
	{ "/img/img_20", "/img/IMG_20" },
	{ "/img/jpilot2-awfuldate", "/img/jpilot2-awfulDate" },
	{ "/img/mthyrs", "/img/MthYrs" },
	{ "/img/openmp-forkjoin", "/img/openMP-ForkJoin" },
	{ "/img/openmp-datasharingattrib", "/img/openMP-DataSharingAttrib" },
	{ "/img/openmp-datasharing", "/img/openMP-DataSharing" },
	{ "/img/openmp-worksharing", "/img/openMP-Worksharing" },
	{ "/img/openmp-reduction", "/img/openMP-Reduction" },
	{ "/img/openmp-tasks", "/img/openMP-Tasks" },
	{ "/img/openmp-latency", "/img/openMP-Latency" },
	{ "/img/pallenebench", "/img/PalleneBench" },
	{ "/img/perfryzenintelarm1-1", "/img/PerfRyzenIntelARM1" },
	{ "/img/soehneweltmachtbild1", "/img/SoehneWeltmachtBild" },
	{ "/img/tempfrequencydomain", "/img/tempFrequencyDomain" },
	{ "/img/topposts2016", "/img/topPosts2016" },
	{ "/img/unitymediaspeed", "/img/UnitymediaSpeed" },
	{ "/img/unitymedia100m", "/img/Unitymedia100M" },
	{ "/img/viewsperc", "/img/viewsPerC" },
	{ "/img/viewsperm", "/img/viewsPerM" },
	{ "/img/viewspery", "/img/viewsPerY" },
	{ "/img/win10-changepwd", "/img/Win10-ChangePwd" },
	{ "/img/win10-idle-after20m", "/img/Win10-Idle-After20m" },
	{ "/img/win10-idle-disaster", "/img/Win10-Idle-Disaster" },
	//{ "", "" },
}

func replaceSpecChar(s string) string {
	s = strings.ReplaceAll(s, "&#038;", "&")
	s = strings.ReplaceAll(s, "&#039;", "'")
	s = strings.ReplaceAll(s, "&#8211;", "--")
	s = strings.ReplaceAll(s, "&#8220;", "\"")
	s = strings.ReplaceAll(s, "&#8221;", "\"")
	s = strings.ReplaceAll(s, "&#8230;", "...")
	s = strings.ReplaceAll(s, "&quot;", "\"")

	return s
}

func cvt2Mkdwn(bodyStr string) string {
	//s0 := string(bodyStr[0:])
	s := bodyStr

	// Early search and replace
	for _, early := range earlySearchReplace {
		s = strings.ReplaceAll(s, early.search, early.replace)
	}

	s = deHTMLify(s)

	// replace attachments with references to /img/attachment
	for k, v := range attachm {
		if strings.Contains(s, k) {
			v.cnt += 1 // attachment is used in body
			//s = strings.Replace(s, k, v.hugoFname, -1)
		}
	}

	// search and replace
	for _, srpl := range searchReplace {
		s = strings.ReplaceAll(s, srpl.search, srpl.replace)
	}
	replaceSpecChar(s)

	// finally process all regex
	for _, r := range srchReplRegEx {
		s = r.regx.ReplaceAllString(s, r.replace)
	}

	// late search and replace
	for _, late := range lateSearchReplace {
		s = strings.ReplaceAll(s, late.search, late.replace)
	}

	return s
}

// Take a list of strings and return concatenated string of it,
// each entry in quotes, separated by comma.
// http://stackoverflow.com/questions/1760757/how-to-efficiently-concatenate-strings-in-go
func commaSep(list map[int]string, prefix string) string {
	lineLen := 0
	line := make([]byte, 2048)

	if len(list) <= 0 {
		return ""
	}

	lineLen += copy(line[lineLen:], prefix)
	lineLen += copy(line[lineLen:], ": [")
	sep := ""
	for key := range list {
		lineLen += copy(line[lineLen:], sep)
		lineLen += copy(line[lineLen:], "\"")
		lineLen += copy(line[lineLen:], list[key])
		lineLen += copy(line[lineLen:], "\"")
		sep = ", "
	}
	lineLen += copy(line[lineLen:], "]\n")
	return string(line[0:lineLen])
}

// Write post/page to appropriate directory
func wrtPostFile(frontmatter map[string]string, cats, tags map[int]string, body []byte, bodyLen int, reblog []byte, reblogLen int) {
	var dirname string
	tx2, post := 0, 0
	if frontmatter["wp:status"] == "draft" {
		dirname = filepath.Join("content", "draft")
	} else if frontmatter["wp:status"] == "private" {
		dirname = filepath.Join("content", "private")
	} else if frontmatter["wp:post_type"] == "post" {
		post = 1
		dirname = filepath.Join("content", "blog")
	} else if frontmatter["wp:post_type"] == "page" {
		dirname = filepath.Join("content", "page")
	} else {
		return // do not write anything if attachment, or nav_menu_item
	}
	// create directory content/post, content/page, etc.
	if os.MkdirAll(dirname, 0775) != nil {
		fmt.Println("Cannot create directory ", dirname)
		os.Exit(11)
	}

	link := frontmatter["link"]
	fname := frontmatter["wp:post_name"]
	// Create directory, e.g., content/blog/2015  (previously: content/post/05/14/)
	if tx2 = strings.Index(link, fname); tx2 > 0 {
		if tx1 := strings.Index(link, wpconfig["baseURL"]); tx1 >= 0 {
			//fmt.Println("\t\ttx1 =",tx1,", tx2 =",tx2,", link =",link,", fname =",fname,", baseURL =",wpconfig["baseURL"])
			if post == 1 {
				tx2 -= 6	// because strlen("/05/04") == 6
			}
			dirname = filepath.Join(dirname, link[tx1+len(wpconfig["baseURL"])+1:tx2])
			//fmt.Println("\t\tdirname =",dirname)
			if len(dirname) > 0 && os.MkdirAll(dirname, 0775) != nil {
				fmt.Println("Cannot create directory ", dirname)
				os.Exit(12)
			}
		}
	}

	// Create file in above directory, e.g., content/post/2015/05/14/my-post.md
	if tx2 > 0 && post == 1 {
		fname = link[tx2:tx2+2] + "-" + link[tx2+3:tx2+5] + "-" + fname
	}
	fname = filepath.Join(dirname, fname) + ".md"
	fout, err := os.Create(fname)
	if err != nil {
		fmt.Println("Cannot open ", fname, " for writing")
		os.Exit(13)
	}

	bodyStr := string(body[0:bodyLen])
	if reblogLen > 0 {
		bodyStr += "\n\n\n-- -- -- Quote -- -- --\n\n" + string(reblog[0:reblogLen])
	}

	catLine := commaSep(cats, "categories")
	tagLine := commaSep(tags, "tags")
	authorLine := ""
	creator := frontmatter["dc:creator"]
	if len(creator) > 0 {
		if creator == "eklausmeier" {
			authorLine = "author: \"Elmar Klausmeier\"\n"
		} else {
			authorLine = "author: \"" + creator + "\"\n"
		}
	}
	mathLine := ""
	if strings.Contains(bodyStr, "$latex ") {
		mathLine = "MathJax: true\n"
	}
	prismjsLine := ""
	if strings.Contains(bodyStr, "[code ") {
		prismjsLine = "prismjs: true\n"
	}

	title := replaceSpecChar(frontmatter["title"])
	//if strings.Contains(title,":") {
	//	title = "\"" + title + "\""
	//}
	title = strings.ReplaceAll(title,"\"","\\\"")

	// Write frontmatter + body
	w := bufio.NewWriter(fout)
	fmt.Fprintf(w, "---\n"+
		"date: \"%s\"\n"+
		"title: \"%s\"\n"+
		"draft: %t\n"+
		"%s"+
		"%s"+
		"%s"+
		"%s"+
		"%s"+
		"---\n\n"+
		"%s\n",
		frontmatter["wp:post_date"],
		title,
		frontmatter["wp:status"] == "draft",
		catLine,
		tagLine,
		authorLine,
		mathLine,
		prismjsLine,
		cvt2Mkdwn(bodyStr))

	w.Flush()
	fout.Close()
}

func wrtConfigToml(configCats map[int]string) {
	fout, err := os.Create("config.toml")
	if err != nil {
		fmt.Println("Cannot open config.toml for writing")
		os.Exit(14)
	}

	//catLine := commaSep(configCats,"categories");

	w := bufio.NewWriter(fout)
	fmt.Fprintf(w, "\n"+
		"title = \"%s\"\n"+
		"languageCode = \"%s\"\n"+
		"baseURL = \"%s\"\n"+
		"paginate = 20\n"+
		"\n"+
		"[taxonomies]\n"+
		"   tag = \"tags\"\n"+
		"   category = \"categories\"\n"+
		"   archive = \"archives\"\n"+
		"\n"+
		"[params]\n"+
		"   description = \"%s\"\n"+
		"\n\n",
		wpconfig["title"],
		wpconfig["language"],
		wpconfig["baseURL"],
		wpconfig["description"])

	w.Flush()
	fout.Close()
}

// Write file attachm.txt with all used attachments.
// This can be used to wget/curl these attachments from the WordPress server.
// For example:
//     perl -ane '`curl $F[0] -o $F[1]\n`' attachm.txt
func wrtAttachm() {
	fout, err := os.Create("attachm.txt")
	if err != nil {
		fmt.Println("Cannot open attachm.txt for writing")
		os.Exit(15)
	}

	w := bufio.NewWriter(fout)
	for k, v := range attachm {
		if v.cnt > 0 && strings.Contains(k, "https://") {
			fmt.Fprintf(w, "%s\t%s\n", k, v.fname)
		}
	}

	w.Flush()
	fout.Close()
}

func wp2hugo(finp *os.File) {
	scanner := bufio.NewScanner(finp)
	inItem, inImage, empty, inBody, inReblog := false, false, false, false, false
	bodyLen, reblogLen, tx1, tx2 := 0, 0, 0, 0
	body := make([]byte, 524288) // 2^19 as a somewhat arbitrary limit on body length
	reblog := make([]byte, 524288) // 2^19 as a somewhat arbitrary limit on reblog length
	frontmatter := make(map[string]string)
	configCatCnt := 0
	configCats := make(map[int]string) // later: categories/taxonomies in config.toml
	catCnt := 0
	cats := make(map[int]string) // goes to categories = ["a","b",...]
	tagCnt := 0
	tags := make(map[int]string) // goes to tags = ["u","v",...]

	for scanner.Scan() { // read each line
		//fmt.Println(scanner.Text())
		s := scanner.Text()
		if !inItem && !inImage {
			if strings.Contains(s, "<image>") {
				inImage = true
			} else if tx1 = strings.Index(s, "<title>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</title>"); tx2 >= tx1 {
					wpconfig["title"] = strings.TrimSpace(s[tx1+7 : tx2])
					//fmt.Println("\t\tinItem =",inItem,", config-title =",wpconfig["title"])
				}
			} else if tx1 = strings.Index(s, "<link>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</link>"); tx2 >= tx1 {
					t := s[tx1+6 : tx2]
					wpconfig["baseURL"] = t // usually this will contain https://
					wpconfig["baseURLalt"] = strings.Replace(t, "http://", "https://", -1)
					wpconfig["baseURLalt"] = strings.Replace(t, "https://", "http://", -1)
				}
			} else if tx1 = strings.Index(s, "<description>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</description>"); tx2 >= tx1 {
					wpconfig["description"] = s[tx1+13 : tx2]
				}
			} else if tx1 = strings.Index(s, "<language>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</language>"); tx2 >= tx1 {
					wpconfig["language"] = s[tx1+10 : tx2]
				}
			} else if tx1 = strings.Index(s, "<wp:cat_name><![CDATA["); tx1 >= 0 {
				if tx2 = strings.Index(s, "]]></wp:cat_name>"); tx2 >= tx1 {
					configCats[configCatCnt] = s[tx1+22 : tx2]
					configCatCnt++
				}
			} else if strings.Contains(s, "<item>") {
				// For each new post frontmatter, categories, etc. are cleared
				inItem, empty, inBody, inReblog, bodyLen, reblogLen = true, false, false, false, 0, 0
				for key := range frontmatter {
					delete(frontmatter, key)
				}
				for key := range cats {
					delete(cats, key)
				}
				for key := range tags {
					delete(tags, key)
				}
				//fmt.Println("\t\tOpening <item> found: inItem =",inItem)
				continue
			}
		} else if strings.Contains(s, "</image>") {
			inImage = false
		} else if strings.Contains(s, "</item>") {
			//fmt.Println("tile=",frontmatter["title"],"link=",frontmatter["link"],"bodyLen=",bodyLen,"wp:post_name=",frontmatter["wp:post_name"])
			inItem = false
			if !empty && bodyLen > 0 && len(frontmatter["wp:post_name"]) > 0 {
				wrtPostFile(frontmatter, cats, tags, body, bodyLen, reblog, reblogLen)
			}
			//fmt.Println("\t\t</item> found: inItem =",inItem)
			continue
		}
		if inItem && !empty {
			if tx1 = strings.Index(s, "<title>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</title>"); tx2 >= tx1 {
					t := strings.TrimSpace(s[tx1+7 : tx2])
					t = strings.Replace(t, "\\", "\\\\", -1) // replace backslash with double backslash
					t = strings.Replace(t, "\"", "\\\"", -1) // replace quote with backslash quote
					frontmatter["title"] = t
				}
			} else if tx1 = strings.Index(s, "<link>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</link>"); tx2 >= tx1 {
					frontmatter["link"] = s[tx1+6 : tx2]
				}
			} else if tx1 = strings.Index(s, "<pubDate>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</pubDate>"); tx2 >= tx1 {
					frontmatter["pubDate"] = s[tx1+9 : tx2]
				}
			} else if tx1 = strings.Index(s, "<dc:creator>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</dc:creator>"); tx2 >= tx1 {
					frontmatter["dc:creator"] = s[tx1+12 : tx2]
				}
			} else if strings.Contains(s, "<wp:post_name/>") && len(frontmatter["title"]) > 0 {
				// convert file name with spaces and special chars to something without that
				t := strings.ToLower(frontmatter["title"])
				nameLen := 0
				name := make([]byte, 256)
				flip := false
				for _, elem := range t {
					if unicode.IsLetter(elem) {
						nameLen += copy(name[nameLen:], string(elem))
						flip = true
					} else if flip {
						nameLen += copy(name[nameLen:], "-")
						flip = false
					}
				}
				if name[nameLen-1] == '-' { // file name ending in '-' looks ugly
					nameLen--
				}
				frontmatter["wp:post_name"] = string(name[0:nameLen])
			} else if tx1 = strings.Index(s, "<wp:post_name>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</wp:post_name>"); tx2 >= tx1 {
					frontmatter["wp:post_name"] = s[tx1+14 : tx2]
				}
			} else if tx1 = strings.Index(s, "<wp:post_date>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</wp:post_date>"); tx2 >= tx1 {
					frontmatter["wp:post_date"] = s[tx1+14 : tx2]
				}
			} else if tx1 = strings.Index(s, "<wp:status>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</wp:status>"); tx2 >= tx1 {
					frontmatter["wp:status"] = s[tx1+11 : tx2] // either: draft, future, inherit, private, publish
				}
			} else if tx1 = strings.Index(s, "<wp:post_id>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</wp:post_id>"); tx2 >= tx1 {
					frontmatter["wp:post_id"] = s[tx1+12 : tx2]
				}
			} else if tx1 = strings.Index(s, "<wp:post_type>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</wp:post_type>"); tx2 >= tx1 {
					frontmatter["wp:post_type"] = s[tx1+14 : tx2] // either: attachment, nav_menu_item, page, post
				}
			} else if tx1 = strings.Index(s, "<wp:attachment_url>"); tx1 >= 0 {
				if tx2 = strings.Index(s, "</wp:attachment_url>"); tx2 >= tx1 {
					url := s[tx1+19 : tx2] // e.g., https://eklausmeier.files.wordpress.com/2013/06/cisco1.png
					//fmt.Println("\t\t",url)
					a := new(attachm_t)
					a.cnt = 0                                     // count number of occurences of this attachment
					a.fname = url[strings.LastIndex(url, "/")+1:] // e.g., cisco1.png
					a.hugoFname = "../../../img/" + a.fname               // e.g., /img/cisco1.png
					attachm[url] = a
					urlAlt := strings.Replace(url, "https://", "http://", -1)
					attachm[urlAlt] = a
				}
			} else if strings.Contains(s, "<category domain=\"category\"") {
				if tx1 = strings.Index(s, "<![CDATA["); tx1 >= 0 {
					if tx2 = strings.Index(s, "]]></category>"); tx2 >= tx1 {
						cats[catCnt] = s[tx1+9 : tx2]
						catCnt++
					}
				}
			} else if strings.Contains(s, "<category domain=\"post_tag\"") {
				if tx1 = strings.Index(s, "<![CDATA["); tx1 >= 0 {
					if tx2 = strings.Index(s, "]]></category>"); tx2 >= tx1 {
						tags[tagCnt] = s[tx1+9 : tx2]
						tagCnt++
					}
				}
				//} else if strings.Contains(s,"<content:encoded><![CDATA[]]></content:encoded>") {
				//	empty = true
			} else if tx2 = strings.Index(s, "]]></content:encoded>"); tx2 >= 0 {
				if tx1 = strings.Index(s, "<content:encoded><![CDATA["); tx1 < 0 {
					tx1 = 0 // here we accumulate body text from previous lines
				} else {
					tx1 += 26 // content is on single line
				}
				if inBody || !strings.Contains(s, "jpg]]></content:encoded>") {
					bodyLen += copy(body[bodyLen:], s[tx1:tx2])
					bodyLen += copy(body[bodyLen:], "\n")
					inBody = false
				}
			} else if tx2 = strings.Index(s, `</div>";`); tx2 >= 0 {
				reblogLen += copy(reblog[reblogLen:], s[0:tx2])
				reblogLen += copy(reblog[reblogLen:], "\n")
				inReblog = false
			} else if tx1 = strings.Index(s, "<content:encoded><![CDATA["); tx1 >= 0 {
				bodyLen += copy(body[bodyLen:], s[tx1+26:])
				bodyLen += copy(body[bodyLen:], "\n")
				inBody = true
			} else if tx1 = strings.Index(s, `<div class="reblogged-content">`); tx1 >= 0 {
				reblogLen += copy(reblog[reblogLen:], s[tx1+31:])
				reblogLen += copy(reblog[reblogLen:], "\n")
				inReblog = true
			} else if inBody {
				bodyLen += copy(body[bodyLen:], s)
				bodyLen += copy(body[bodyLen:], "\n")
			} else if inReblog {
				reblogLen += copy(reblog[reblogLen:], s)
				reblogLen += copy(reblog[reblogLen:], "\n")
			}
		}
	}

	wrtConfigToml(configCats) // write config.toml
	wrtAttachm()

	//os.MkdirAll("layouts", 0775)
	//os.MkdirAll("archetypes", 0775)
	//os.MkdirAll("static", 0775)
	//os.MkdirAll("data", 0775)
	//os.MkdirAll("themes", 0775)
}

func main() {
	if len(os.Args) <= 1 {
		fmt.Println("Expect at least one file-name as argument")
		os.Exit(16)
	}

	// WordPress export may contain multiple files
	for _, i := range os.Args[1:] {
		fmt.Println("\tWorking on file: ", i)
		finp, err := os.Open(i) // for read access
		if err != nil {
			fmt.Println("\tCannot open file ", i, "/", err)
		} else {
			wp2hugo(finp)
		}
		finp.Close()
	}
}

