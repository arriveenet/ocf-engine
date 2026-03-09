#pragma once
#include <string>
#include <vector>

namespace ocf {

/**
 * @brief ファイルユーティリティクラス
 */
class FileUtils {
public:
    /** FileUtilsのインスタンスを取得する */
    static FileUtils* getInstance();

    /** FileUtilsのインスタンスを破棄する */
    static void destroyInstance();

    /**
     * @brief ファイル名から拡張子を取得する
     * @param filename ファイル名
     * @return 拡張子
     */
    static std::string getExtension(std::string_view filename);

    /** デストラクター */
    ~FileUtils();

    /**
     * @brief ファイル名から絶対パスを取得する
     * @param filename ファイル名
     * @return 変換された絶対パス
     */
    std::string getFullPath(const std::string& filename) const;

    /**
     * @brief ファイル名から親の絶対パスを取得する
     * @param filename ファイル名
     * @return 変換された親の絶対パス
     */
    std::string getParentFullPath(const std::string& filename) const;

    /**
     * @brief サーチパスにパスを追加する
     * @param path 追加するパス
     * @param front 一番前に追加するかどうか
     */
    void addSearchPath(const std::string& path, bool front = false);

    /**
     * @brief サーチパスを取得する
     * @return サーチパス
     */
    const std::vector <std::string>& getSearchPath() const;

    /**
     * @brief assetsフォルダのパスを取得する
     * @return assetsフォルダのパス
     */
    std::string getAssetsPath() const;

    /**
     * @brief ファイル名からフルパスを取得する
     * @param ファイル名
     * @return フルパス。存在しなかった場合空文字を返却する
     */
    std::string fullPathForFilename(const std::string& filename) const;

    /**
     * @brief ファイル内の文字列を取得する
     * @param ファイル名
     * @return ファイルのコンテンツ
     */
    std::string getStringFromFile(std::string_view filename) const;

    /**
     * @brief ファイルが存在するかチェックする
     * @param ファイル名
     * @return ファイルが存在する場合true、存在しない場合falseを返却
     */
    bool isFileExist(const std::string& filename) const;

protected:
    /** デフォルトコンストラクター */
    FileUtils();

    /**
     * @brief FileUtilsのインスタンスを初期化する
     */
    bool init();

    /**
     * @brief ファイル名からパスを取得
     * @param filename ファイル名
     * @param searchPath 検索パス
     * @return ファイルが存在した場合そのパスを返却、存在しなかった場合空文字を返却する
     */
    std::string getPathForFilename(const std::string& filename, const std::string& searchPath) const;

private:
    /** FileUtilsのシングルトンポインター */
    static FileUtils* s_sharedFileUtils;
    
    /** exeのあるディレクトリ */
    static std::string s_exeDirectory;

    /** ファイルを検索するパスの配列 */
    std::vector<std::string> m_searchPathArray;
    /** デフォルトのassetsのルートパス */
    std::string m_defaultAssetsRootPath;
};

} // namespace ocf