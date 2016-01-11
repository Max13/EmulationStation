#include "views/gamelist/BasicGameListView.h"
#include "views/ViewController.h"
#include "Renderer.h"
#include "Window.h"
#include "ThemeData.h"
#include "SystemData.h"
#include "Settings.h"
#include "Log.h"


BasicGameListView::BasicGameListView(Window* window, FileData* root)
	: ISimpleGameListView(window, root), mList(window)
{
	LOG(LogDebug) << "BasicGameListView::BasicGameListView()";
	mList.setSize(mSize.x(), mSize.y() * 0.8f);
	mList.setPosition(0, mSize.y() * 0.2f);
	addChild(&mList);

	populateList(root->getChildren());
}

void BasicGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);
	using namespace ThemeFlags;
	mList.applyTheme(theme, getName(), "gamelist", ALL);
}

void BasicGameListView::onFileChanged(FileData* file, FileChangeType change)
{
	if(change == FILE_METADATA_CHANGED)
	{
		// might switch to a detailed view
		ViewController::get()->reloadGameListView(this);
		return;
	}

	ISimpleGameListView::onFileChanged(file, change);
}

void BasicGameListView::populateList(const std::vector<FileData*>& files)
{
	LOG(LogDebug) << "BasicGameListView::populateList()";
	mList.clear();
	LOG(LogDebug) << "BasicGameListView::populateList():0";
	mHeaderText.setText(files.at(0)->getSystem()->getFullName());
	LOG(LogDebug) << "BasicGameListView::populateList():1";
	bool hasFavorites = false;
	bool hasKidGames = false;
	if (Settings::getInstance()->getBool("FavoritesOnly"))
	{
		LOG(LogDebug)<< "trying to find fav games";
		for (auto it = files.begin(); it != files.end(); it++)
		{
			if ((*it)->getType() == GAME)
			{
				if ((*it)->metadata.get("favorite").compare("true") == 0)
				{
					hasFavorites = true;
					LOG(LogDebug)<< "at least 1 fav game found!";

					break;
				}
			}
		}
		LOG(LogDebug) << "BasicGameListView::populateList()2";
	}else if (Settings::getInstance()->getString("UIMode") == "Kid")
	{
		LOG(LogDebug)<< "trying to find kid games";

		for (auto it = files.begin(); it != files.end(); it++)
		{
			if ((*it)->getType() == GAME)
			{
				if ((*it)->metadata.get("kidgame").compare("true") == 0)
				{
					hasKidGames = true;
					LOG(LogDebug)<< "at least 1 kid game found!";

					break;
				}
			}
		}
	}
	LOG(LogDebug) << "BasicGameListView::populateList():3";
	for(auto it = files.begin(); it != files.end(); it++)
	{
		if ((*it)->getType() == GAME)
		{
			if (Settings::getInstance()->getString("UIMode") == "Full")
			{
				LOG(LogDebug) << "BasicGameListView::populateList():4";

				if (Settings::getInstance()->getBool("FavoritesOnly") && hasFavorites)
				{
					LOG(LogDebug) << "BasicGameListView::populateList():5";
					if((*it)->metadata.get("favorite").compare("true") == 0)
					{
						LOG(LogDebug) << "UImode=full, Fav only";
						mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}
				}else 
				{
					LOG(LogDebug) << "BasicGameListView::populateList():6";
					//LOG(LogDebug) << "UImode=full";
					mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
				}
			}else if (Settings::getInstance()->getString("UIMode") == "Kiosk") // filter hidden
			{
				LOG(LogDebug) << "BasicGameListView::populateList():7";
				if (Settings::getInstance()->getBool("FavoritesOnly") && hasFavorites)
				{
					LOG(LogDebug) << "BasicGameListView::populateList():8";
					if(((*it)->metadata.get("favorite").compare("true") == 0) &&
					   ((*it)->metadata.get("hidden").compare("false") == 0))
					{
						LOG(LogDebug) << "UImode=kiosk, Fav only & !hidden";
						mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}
				}else 
				{
					LOG(LogDebug) << "BasicGameListView::populateList():9";
					if((*it)->metadata.get("hidden").compare("false") == 0)
					{
						LOG(LogDebug) << "UImode=kiosk, !hidden";
						mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}
				}
			}else if ((Settings::getInstance()->getString("UIMode") == "Kid") && hasKidGames) // filter all non kid-game
			{
				LOG(LogDebug) << "BasicGameListView::populateList():10";
				if (Settings::getInstance()->getBool("FavoritesOnly") && hasFavorites)
				{
					if(((*it)->metadata.get("favorite").compare("true") == 0) &&
					   ((*it)->metadata.get("hidden").compare("false") == 0) &&
					   ((*it)->metadata.get("kidgame").compare("true") == 0))
					{
						LOG(LogDebug) << "UImode=kid, favonly, kidonly, !hidden";
						mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}
				}else 
				{
					LOG(LogDebug) << "BasicGameListView::populateList():11";
					if(((*it)->metadata.get("hidden").compare("false") == 0) &&
					   ((*it)->metadata.get("kidgame").compare("true") == 0))
					{
						LOG(LogDebug) << "UImode=kid, favonly, kidonly, !hidden";
						mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}
				}
			}
		}
	}
	LOG(LogDebug) << "BasicGameListView::populateList():done";
}

FileData* BasicGameListView::getCursor()
{
	return mList.getSelected();
}

void BasicGameListView::setCursor(FileData* cursor)
{
	if(!mList.setCursor(cursor))
	{
		populateList(cursor->getParent()->getChildren());
		mList.setCursor(cursor);

		// update our cursor stack in case our cursor just got set to some folder we weren't in before
		if(mCursorStack.empty() || mCursorStack.top() != cursor->getParent())
		{
			std::stack<FileData*> tmp;
			FileData* ptr = cursor->getParent();
			while(ptr && ptr != mRoot)
			{
				tmp.push(ptr);
				ptr = ptr->getParent();
			}
			
			// flip the stack and put it in mCursorStack
			mCursorStack = std::stack<FileData*>();
			while(!tmp.empty())
			{
				mCursorStack.push(tmp.top());
				tmp.pop();
			}
		}
	}
}

void BasicGameListView::launch(FileData* game)
{
	ViewController::get()->launch(game);
}

std::vector<HelpPrompt> BasicGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if(Settings::getInstance()->getBool("QuickSystemSelect"))
		prompts.push_back(HelpPrompt("left/right", "system"));
	prompts.push_back(HelpPrompt("up/down", "choose"));
	prompts.push_back(HelpPrompt("a", "launch"));
	prompts.push_back(HelpPrompt("b", "back"));
	prompts.push_back(HelpPrompt("select", "options"));
	return prompts;
}
