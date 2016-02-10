# -*- coding: utf-8 -*-

from __future__ import unicode_literals
import time

OUTPUT_FOLDER = '.output'
CACHE_FOLDER = '.cache'
USE_BUNDLES = False

GLOBAL_CONTEXT = {}
GLOBAL_CONTEXT_FILLER = []

REDIRECTIONS = []

BLOG_AUTHOR = 'Swanson Project'
BLOG_DESCRIPTION = 'Swanson programming language'
BLOG_TITLE = 'Swanson'
SITE_URL = 'https://www.swanson-lang.org/'

DEFAULT_LANG = 'en'
TRANSLATIONS = {
    DEFAULT_LANG: '',
}
TRANSLATIONS_PATTERN = '{path}.{lang}.{ext}'

TIMEZONE = 'UTC'
FORCE_ISO8601 = True
DATE_FANCINESS = 2

NAVIGATION_LINKS = {
    #DEFAULT_LANG: (
    #    ('/archive.html', 'Archives'),
    #    ('/categories/index.html', 'Tags'),
    #    ('/rss.xml', 'RSS feed'),
    #),
}

THEME = 'swanson'

POSTS = ()
PAGES = (('content/*.rst', '', 'story.tmpl'),)

COMPILERS = {
    'rest': ('.rst', '.txt'),
}

CATEGORY_ALLOW_HIERARCHIES = False
CATEGORY_OUTPUT_FLAT_HIERARCHY = False
COMMENT_SYSTEM = None
ENABLE_AUTHOR_PAGES = False
FEED_LINKS_APPEND_QUERY = False
FEED_READ_MORE_LINK = '<p><a href="{link}">{read_more}…</a> ({min_remaining_read})</p>'
FEED_TEASERS = False
GALLERY_FOLDERS = {}
HIDDEN_CATEGORIES = []
IMAGE_FOLDERS = {}
INDEX_READ_MORE_LINK = '<p class="more"><a href="{link}">{read_more}…</a></p>'
INDEX_PATH = 'blog'
LICENSE = ''
POSTS_SECTIONS = False
PRETTY_URLS = True
SHOW_SOURCELINK = False
STRIP_INDEXES = True
UNSLUGIFY_TITLES = True
WRITE_TAG_CLOUD = False
