/*
 * Copyright (c) 2022-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk.samples

import android.content.*
import android.database.Cursor
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteException
import android.database.sqlite.SQLiteOpenHelper
import android.database.sqlite.SQLiteQueryBuilder
import android.net.Uri

class CheeseyContentProvider : ContentProvider() {
    companion object {
        const val PROVIDER_NAME = "com.newrelic.agent.android.ndk.samples.provider"
        const val PATH = "cheese"
        const val URL = "content://$PROVIDER_NAME/$PATH"

        val CONTENT_URI = Uri.parse(URL)

        val id = "id";
        val country = "country";
        val name = "name";
    }

    lateinit var db: SQLiteDatabase

    val uriCode = 1;
    var uriMatcher = UriMatcher(UriMatcher.NO_MATCH);
    val projections = mutableMapOf<String, String>()

    init {
        uriMatcher.addURI(PROVIDER_NAME, PATH, uriCode);
        uriMatcher.addURI(PROVIDER_NAME, PATH, uriCode);
        uriMatcher.addURI(PROVIDER_NAME, "$PATH/*", uriCode);
    }

    internal class DatabaseHelper(context: Context?) :
        SQLiteOpenHelper(context, DATABASE_NAME, null, 1) {

        companion object {
            const val DATABASE_NAME = "DidYouTryTheCheese"
            const val TABLE_NAME = "Cheese"
            const val CREATE_DB_TABLE = ("CREATE TABLE " + TABLE_NAME
                    + " (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    + " country TEXT NOT NULL, "
                    + " name TEXT NOT NULL);")

            val STOCK = mutableMapOf<String, String>()

            init {
                STOCK.put("UK", "Wendsleydale")
                STOCK.put("US", "Velveeta")
                STOCK.put("FR", "Brie")
                STOCK.put("MX", "Queso")
            }
        }

        override fun onCreate(db: SQLiteDatabase) {
            db.execSQL(CREATE_DB_TABLE)
            STOCK.forEach { k, v ->
                val contentValues = ContentValues()
                contentValues.put(CheeseyContentProvider.country, k)
                contentValues.put(CheeseyContentProvider.name, v)
                db.insert(TABLE_NAME, null, contentValues)
            }
        }

        override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
            db.execSQL("DROP TABLE IF EXISTS $TABLE_NAME")
            onCreate(db)
        }
    }

    override fun onCreate(): Boolean {
        db = DatabaseHelper(context).getWritableDatabase()

        return true
    }

    override fun query(
        uri: Uri,
        projection: Array<out String>?,
        selection: String?,
        selectionArgs: Array<out String>?,
        sortOrder: String?
    ): Cursor? {
        Thread.sleep(5000)

        val qb = SQLiteQueryBuilder()
        qb.tables = DatabaseHelper.TABLE_NAME

        when (uriMatcher.match(uri)) {
            uriCode -> qb.setProjectionMap(projections)
            else -> throw java.lang.IllegalArgumentException("Unknown URI $uri")
        }

        val c = qb.query(db, projection, selection, selectionArgs, null, null, id)

        c.setNotificationUri(context.contentResolver, uri)

        return c
    }

    override fun getType(uri: Uri): String? {
        Thread.sleep(5000)
        return when (uriMatcher.match(uri)) {
            uriCode -> "vnd.android.cursor.dir/$PATH"
            else -> throw IllegalArgumentException("Unsupported URI: $uri")
        }
    }

    override fun insert(uri: Uri, values: ContentValues?): Uri? {
        Thread.sleep(5000)
        val rowID: Long = db.insert(PATH, "", values)
        if (rowID > 0) {
            val _uri = ContentUris.withAppendedId(CONTENT_URI, rowID)
            context.contentResolver.notifyChange(_uri, null)
            return _uri
        }
        throw SQLiteException("Failed to add a record into $uri")

        return Uri.parse("Snoozed")
    }

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<out String>?): Int {
        Thread.sleep(5000)
        var count = 0
        count =
            when (uriMatcher.match(uri)) {
                uriCode -> db.delete(PATH, selection, selectionArgs)
                else -> throw java.lang.IllegalArgumentException("Unknown URI $uri")
            }
        context.contentResolver.notifyChange(uri, null)
        return count
    }

    override fun update(
        uri: Uri,
        values: ContentValues?,
        selection: String?,
        selectionArgs: Array<out String>?
    ): Int {
        var count = 0

        Thread.sleep(5000)
        when (uriMatcher.match(uri)) {
            uriCode -> count = db.update(PATH, values, selection, selectionArgs)
            else -> throw java.lang.IllegalArgumentException("Unknown URI $uri")
        }
        context.contentResolver.notifyChange(uri, null)

        return count
    }

}
